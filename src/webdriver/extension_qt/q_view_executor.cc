#include "extension_qt/q_view_executor.h"

#include "webdriver_logging.h"
#include "webdriver_session.h"
#include "q_key_converter.h"
#include "extension_qt/widget_view_handle.h"
#include "widget_view_util.h"

#include <QtCore/QDebug>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#else
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>
#endif

namespace webdriver {

QViewCmdExecutor::QViewCmdExecutor(Session* session, ViewId viewId)
    : ViewCmdExecutor(session, viewId) {

}

QViewCmdExecutor::~QViewCmdExecutor() {

};

QWidget* QViewCmdExecutor::getView(const ViewId& viewId, Error** error) {
    QWidget* pWidget = QWidgetViewUtil::getView(session_, viewId);

    if (NULL == pWidget) {
        session_->logger().Log(kWarningLogLevel, "checkView - no such view("+viewId.id()+")");
        *error = new Error(kNoSuchWindow);
        return NULL;
    }

    return pWidget;
}

void QViewCmdExecutor::GetTitle(std::string* title, Error **error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    *title = view->windowTitle().toStdString();

    session_->logger().Log(kFineLogLevel, "GetTitle - "+*title);
}

void QViewCmdExecutor::GetWindowName(std::string* name, Error ** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    *name = view->windowTitle().toStdString();

    session_->logger().Log(kFineLogLevel, "GetWindowName - "+*name);
}

void QViewCmdExecutor::GetBounds(Rect *bounds, Error **error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    *bounds = ConvertQRectToRect(view->geometry());
}
    
void QViewCmdExecutor::SetBounds(const Rect& bounds, Error** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    view->setGeometry(ConvertRectToQRect(bounds));
}

void QViewCmdExecutor::Maximize(Error** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    if (!view->isTopLevel()) {
        *error = new Error(kUnknownError, "Cant maximize non top level view.");
        return;
    }

    view->setGeometry(QApplication::desktop()->rect());    
}

void QViewCmdExecutor::GetScreenShot(std::string* png, Error** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;
    
    const FilePath::CharType kPngFileName[] = FILE_PATH_LITERAL("./screen.png");
    FilePath path = session_->temp_dir().Append(kPngFileName);;

    QPixmap pixmap = QPixmap::grabWidget(view);

    session_->logger().Log(kInfoLogLevel, "Save screenshot to - "+path.value());

#if defined(OS_POSIX)
    if (!pixmap.save(path.value().c_str())) 
#elif defined(OS_WIN)
    if (!pixmap.save(QString::fromUtf16((ushort*)path.value().c_str())))
#endif // OS_WIN
    {
        *error = new Error(kUnknownError, "screenshot was not captured");
        return;
    }

    if (!file_util::ReadFileToString(path, png))
        *error = new Error(kUnknownError, "Could not read screenshot file");
}

void QViewCmdExecutor::SendKeys(const string16& keys, Error** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    std::string err_msg;
    std::vector<QKeyEvent> key_events;
    int modifiers = session_->get_sticky_modifiers();

    if (!QKeyConverter::ConvertKeysToWebKeyEvents(keys,
                               session_->logger(),
                               false,
                               &modifiers,
                               &key_events,
                               &err_msg)) {
        session_->logger().Log(kSevereLogLevel, "SendKeys - cant convert keys:"+err_msg);
        *error = new Error(kUnknownError, "SendKeys - cant convert keys:"+err_msg);
        return;
    }

    session_->set_sticky_modifiers(modifiers);

    std::vector<QKeyEvent>::iterator it = key_events.begin();
    while (it != key_events.end()) {
        qApp->sendEvent(view, &(*it));
        ++it;
    }
}

void QViewCmdExecutor::Close(Error** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    if (!view->isTopLevel()) {
        *error = new Error(kUnknownError, "Cant close non top level view.");
        return;
    }

    session_->logger().Log(kInfoLogLevel, "close View("+view_id_.id()+")");

    session_->RemoveView(view_id_);

    // destroy children correctly
    QList<QWidget*> childs = view->findChildren<QWidget*>();
    foreach(QWidget *child, childs)
    {
        child->setAttribute(Qt::WA_DeleteOnClose, true);
        child->close();
    }

    view->close();
}

void QViewCmdExecutor::SwitchTo(Error** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    session_->set_current_view(view_id_);

    session_->logger().Log(kInfoLogLevel, "SwitchTo - set current view ("+view_id_.id()+")");
}

void QViewCmdExecutor::GetAlertMessage(std::string* text, Error** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    // QMessageBox::information(pWeb, "Alert", message->c_str(), QMessageBox::Ok);
    QMessageBox *msgBox = view->findChild<QMessageBox*>();
    if (NULL != msgBox) {
        *text = msgBox->text().toStdString();
    } else {
        QInputDialog *msgbox = view->findChild<QInputDialog*>();

        if (NULL != msgbox) {
            *text = msgbox->labelText().toStdString();
        } else {
            *error = new Error(kNoAlertOpenError);
        }
    }
}

void QViewCmdExecutor::SetAlertPromptText(const std::string& alert_prompt_text, Error** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    std::string message_text;
    GetAlertMessage(&message_text, error);
    if (*error)
        return;

    QInputDialog *alert = view->findChild<QInputDialog*>();

    if (NULL != alert) {
        alert->setTextValue(alert_prompt_text.c_str());
    } else {
        // in python ELEMENT_NOT_VISIBLE = 11
        // kNoAlertError = 27
        *error = new Error(kElementNotVisible);
    }
}

void QViewCmdExecutor::AcceptOrDismissAlert(bool accept, Error** error) {
    QWidget* view = getView(view_id_, error);
    if (NULL == view)
        return;

    QMessageBox *msgBox = view->findChild<QMessageBox*>();

    if(NULL != msgBox) {
        if(accept) {
            msgBox->accept();
        } else {
            msgBox->close();
        }
    } else {
        QInputDialog *msgbox = view->findChild<QInputDialog*>();
        if(NULL != msgbox) {
            if(accept) {
                msgbox->accept();
            } else {
                msgbox->close();
            }
        } else {
            *error = new Error(kNoAlertOpenError);
        }
    }
}

Rect QViewCmdExecutor::ConvertQRectToRect(const QRect &rect) {
    return Rect(rect.x(), rect.y(), rect.width(), rect.height());
}

QRect QViewCmdExecutor::ConvertRectToQRect(const Rect &rect) {
    QRect resultRect;
    resultRect.setX(rect.x());
    resultRect.setY(rect.y());
    resultRect.setWidth(rect.width());
    resultRect.setHeight(rect.height());

    return resultRect;
}

QPoint QViewCmdExecutor::ConvertPointToQPoint(const Point &p) {
    QPoint resultPoint;
    resultPoint.setX(p.x());
    resultPoint.setY(p.y());

    return resultPoint;
}

Qt::MouseButton QViewCmdExecutor::ConvertMouseButtonToQtMouseButton(MouseButton button) {
    Qt::MouseButton result = Qt::NoButton;

    switch(button)
    {
        case kLeftButton: result = Qt::LeftButton; break;
        case kMiddleButton: result = Qt::MidButton; break;
        case kRightButton: result = Qt::RightButton; break;
        default: result = Qt::NoButton;
    }

    return result;
}

} // namespace webdriver     