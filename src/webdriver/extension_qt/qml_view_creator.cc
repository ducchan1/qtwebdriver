#include "extension_qt/qml_view_creator.h"

#include "webdriver_session.h"
#include "webdriver_logging.h"
#include "webdriver_error.h"

#include "qml_view_util.h"
#include "extension_qt/widget_view_handle.h"
#include "q_content_type_resolver.h"
#include "q_event_filter.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtCore/QString>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

#include <QtDeclarative/QDeclarativeView>

namespace webdriver {

QQmlViewCreator::QQmlViewCreator() {}

bool QQmlViewCreator::CreateViewByClassName(const Logger& logger, const std::string& className, ViewHandle** view) const {
	ViewHandle* handle = NULL;

    if (factory.empty())
        return false;

	if (className.empty() || className == "QMLView") {
		// get first found QML view
        CreateViewMethod createMethod = factory.begin()->second;
        handle = new QViewHandle(static_cast<QWidget*>(createMethod()));
	} else {
    	FactoryMap::const_iterator it = factory.find(className);
        if (it != factory.end())
        {
        	CreateViewMethod createMethod = it->second;
            handle = new QViewHandle(static_cast<QWidget*>(createMethod()));
        }
    }

    if (NULL != handle) {
        QWidget* widget = (dynamic_cast<QViewHandle*>(handle))->get();
        
        if (NULL != widget) {
            std::string objClassName(widget->metaObject()->className());
            QEventLoop loop;
            QRepaintEventFilter filter(widget);
            QCheckPagePaint painter;
            QObject::connect(&filter, SIGNAL(repainted()), &loop, SLOT(quit()));
            QObject::connect(&filter, SIGNAL(repainted()), &painter, SLOT(pagePainted()));

            QTimer timer;
            timer.setSingleShot(true);
            QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
            timer.start(500);

            widget->installEventFilter(&filter);
            widget->show();
            if (!painter.isPainting())
                loop.exec();
        
            logger.Log(kInfoLogLevel, "QQmlViewCreator created view (" + objClassName +").");

            widget->setAttribute(Qt::WA_DeleteOnClose, true);

            *view = handle;

            return true;
        } else {
            logger.Log(kSevereLogLevel, "QQmlViewCreator, smth wrong.");
            handle->Release();
        }
    }

    // view was not created
    return false;
}

bool QQmlViewCreator::CreateViewForUrl(const Logger& logger, const std::string& url, ViewHandle** view) const {
    if (!QQmlViewUtil::isUrlSupported(url)) {
        return false;
    }
    
    return CreateViewByClassName(logger, "", view);
}

} // namespace webdriver