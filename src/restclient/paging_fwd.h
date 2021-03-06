#ifndef QTRESTCLIENT_PAGING_FWD_H
#define QTRESTCLIENT_PAGING_FWD_H

#include "QtRestClient/qtrestclient_global.h"

#include "QtRestClient/ipaging.h"
#include "QtRestClient/restclient.h"
#include "QtRestClient/restreply.h"
#include "QtRestClient/metacomponent.h"

#include <functional>

#include <QtCore/qshareddata.h>
#include <QtCore/qloggingcategory.h>

#include <QtJsonSerializer/exception.h>

namespace QtRestClient {

template<typename DO, typename EO>
class GenericRestReply;

namespace __private {
template<typename T>
class PagingData;
}

//! A class to access generic paging objects
template<typename T>
class Paging
{
public:
	//! Default Constructor
	Paging();
	//! Copy Constructor
	Paging(const Paging<T> &other);
	//! Move Constructor
	Paging(Paging<T> &&other) noexcept;
	//! Copy assignment operator
	Paging<T> &operator=(const Paging<T> &other);
	//! Move assignment operator
	Paging<T> &operator=(Paging<T> &&other) noexcept;
	//! Constructs a paging from the interface, the data and a client
	Paging(IPaging *iPaging, const QList<T> &data, RestClient *client);

	//! Returns true, if the current paging object is a valid one
	bool isValid() const;
	//! Returns the internally used IPaging instance
	IPaging *iPaging() const;

	//! @copybrief IPaging::items
	QList<T> items() const;

	//! @copybrief IPaging::total
	qint64 total() const;
	//! @copybrief IPaging::offset
	qint64 offset() const;

	//! @copybrief IPaging::hasNext
	bool hasNext() const;
	//! Performs a request for the next paging object
	template<typename EO = QObject*>
	GenericRestReply<Paging<T>, EO> *next() const;
	//! @copybrief IPaging::next
	QUrl nextUrl() const;

	//! @copybrief IPaging::hasPrevious
	bool hasPrevious() const;
	//! Performs a request for the previous paging object
	template<typename EO = QObject*>
	GenericRestReply<Paging<T>, EO> *previous() const;
	//! @copybrief IPaging::previous
	QUrl previousUrl() const;

	//! Iterates over all paging objects
	void iterate(const std::function<bool(T, qint64)> &iterator,
				 qint64 to = -1,
				 qint64 from = 0) const;
	//! @copybrief Paging::iterate(const std::function<bool(T, qint64)> &, qint64, qint64) const
	void iterate(QObject *scope,
				 const std::function<bool(T, qint64)> &iterator,
				 qint64 to = -1,
				 qint64 from = 0) const;
	//! Iterates over all paging objects, with error handling
	template<typename EO = QObject*>
	void iterate(const std::function<bool(T, qint64)> &iterator,
				 const std::function<void(QString, int, RestReply::Error)> &errorHandler,
				 const std::function<QString(EO, int)> &failureTransformer = {},
				 qint64 to = -1,
				 qint64 from = 0) const;
	//! @copybrief Paging::iterate(const std::function<bool(T, qint64)> &, const std::function<void(QString, int, RestReply::Error)> &, const std::function<QString(EO, int)> &, qint64, qint64) const
	template<typename EO = QObject*>
	void iterate(QObject *scope,
				 const std::function<bool(T, qint64)> &iterator,
				 const std::function<void(QString, int, RestReply::Error)> &errorHandler,
				 const std::function<QString(EO, int)> &failureTransformer = {},
				 qint64 to = -1,
				 qint64 from = 0) const;
	//! @copybrief Paging::iterate(const std::function<bool(T, qint64)> &, const std::function<void(QString, int, RestReply::Error)> &, const std::function<QString(EO, int)> &, qint64, qint64) const
	template<typename EO = QObject*>
	void iterate(const std::function<bool(T, qint64)> &iterator,
				 const std::function<void(int, EO)> &failureHandler,
				 const std::function<void(QString, int, RestReply::Error)> &errorHandler = {},
				 const std::function<void(QtJsonSerializer::Exception &)> &exceptionHandler = {},
				 qint64 to = -1,
				 qint64 from = 0) const;
	//! @copybrief Paging::iterate(const std::function<bool(T, qint64)> &, const std::function<void(int, EO)> &, const std::function<void(QString, int, RestReply::Error)> &, const std::function<void(QtJsonSerializer::Exception &)> &, qint64, qint64) const
	template<typename EO = QObject*>
	void iterate(QObject *scope,
				 const std::function<bool(T, qint64)> &iterator,
				 const std::function<void(int, EO)> &failureHandler,
				 const std::function<void(QString, int, RestReply::Error)> &errorHandler = {},
				 const std::function<void(QtJsonSerializer::Exception &)> &exceptionHandler = {},
				 qint64 to = -1,
				 qint64 from = 0) const;

	//! @copybrief IPaging::properties
	QVariantMap properties() const;

	//! Deletes all items this paging object is holding (QObjects only)
	void deleteAllItems() const;

private:
	QSharedDataPointer<__private::PagingData<T>> d;

	qint64 internalIterate(const std::function<bool(T, qint64)> &iterator, qint64 to, qint64 from) const;
	qint64 calcMax(qint64 to) const;
};

Q_RESTCLIENT_EXPORT Q_DECLARE_LOGGING_CATEGORY(logPaging)

}

#endif // QTRESTCLIENT_PAGING_FWD_H
