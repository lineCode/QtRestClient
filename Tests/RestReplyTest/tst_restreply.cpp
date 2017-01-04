#include "tst_global.h"

#include <jphpost.h>

class RestReplyTest : public QObject
{
	Q_OBJECT

signals:
	void testPagingIterate_unlock();

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void testReplyWrapping_data();
	void testReplyWrapping();
	void testReplyError();
	void testReplyRetry();

	void testGenericReplyWrapping_data();
	void testGenericReplyWrapping();

	void testGenericListReplyWrapping_data();
	void testGenericListReplyWrapping();

	void testGenericPagingReplyWrapping_data();
	void testGenericPagingReplyWrapping();
	void testPagingNext();
	void testPagingPrevious();
	void testPagingIterate();

private:
	QNetworkAccessManager *nam;
	QtRestClient::RestClient *client;
};

void RestReplyTest::initTestCase()
{
	initTestJsonServer("./paging-test-db.js");
	nam = new QNetworkAccessManager(this);
	client = new QtRestClient::RestClient(this);
	client->setBaseUrl(QStringLiteral("http://localhost:3000"));
}

void RestReplyTest::cleanupTestCase()
{
	nam->deleteLater();
	nam = nullptr;
	client->deleteLater();
	client = nullptr;
}

void RestReplyTest::testReplyWrapping_data()
{
	QTest::addColumn<QUrl>("url");
	QTest::addColumn<bool>("succeed");
	QTest::addColumn<int>("status");
	QTest::addColumn<QJsonObject>("result");

	QJsonObject object;
	object["userId"] = 1;
	object["id"] = 1;
	object["title"] = "Title1";
	object["body"] = "Body1";

	QTest::newRow("get") << QUrl("http://localhost:3000/posts/1")
						 << true
						 << 200
						 << object;

	QTest::newRow("notFound") << QUrl("http://localhost:3000/posts/baum")
							  << false
							  << 404
							  << QJsonObject();
}

void RestReplyTest::testReplyWrapping()
{
	QFETCH(QUrl, url);
	QFETCH(bool, succeed);
	QFETCH(int, status);
	QFETCH(QJsonObject, result);

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	bool called = false;

	auto reply = new QtRestClient::RestReply(nam->get(request));
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *rep, int code, QJsonObject data){
		called = true;
		QVERIFY(succeed);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QCOMPARE(data, result);
	});
	reply->onFailed([&](QtRestClient::RestReply *rep, int code, QJsonObject data){
		called = true;
		QVERIFY(!succeed);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QCOMPARE(data, result);
	});
	reply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		called = true;
		QFAIL(qUtf8Printable(error));
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteSpy.wait());
	QVERIFY(called);
}

void RestReplyTest::testReplyError()
{
	QNetworkRequest request(QStringLiteral("https://invalid.jsonplaceholder.typicode.com"));
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	bool called = false;

	auto reply = new QtRestClient::RestReply(nam->get(request));
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *, int, QJsonObject){
		called = true;
		QFAIL("succeed with non existant domain");
	});
	reply->onFailed([&](QtRestClient::RestReply *, int, QJsonObject){
		called = true;
		QFAIL("succeed with non existant domain");
	});
	reply->onError([&](QtRestClient::RestReply *rep, QString, int code, QtRestClient::RestReply::ErrorType type) {
		called = true;
		QCOMPARE(rep, reply);
		QCOMPARE(code, (int)QNetworkReply::HostNotFoundError);
		QCOMPARE(type, QtRestClient::RestReply::NetworkError);
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteSpy.wait());
	QVERIFY(called);
}

void RestReplyTest::testReplyRetry()
{
	QNetworkRequest request(QStringLiteral("https://invalid.jsonplaceholder.typicode.com"));
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	auto retryCount = 0;

	auto reply = new QtRestClient::RestReply(nam->get(request));
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *, int, QJsonObject){
		retryCount = 42;
		QFAIL("succeed with non existant domain");
	});
	reply->onFailed([&](QtRestClient::RestReply *, int, QJsonObject){
		retryCount = 42;
		QFAIL("succeed with non existant domain");
	});
	reply->onError([&](QtRestClient::RestReply *rep, QString, int code, QtRestClient::RestReply::ErrorType type) {
		retryCount++;
		QCOMPARE(rep, reply);
		QCOMPARE(code, (int)QNetworkReply::HostNotFoundError);
		QCOMPARE(type, QtRestClient::RestReply::NetworkError);
		if(retryCount < 3)
			rep->retryAfter((retryCount - 1) * 1500);//first 0, the 1500
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(!deleteSpy.wait(1000));
	QVERIFY(deleteSpy.wait(14000));
	QVERIFY(retryCount);
	QCOMPARE(retryCount, 3);
}

void RestReplyTest::testGenericReplyWrapping_data()
{
	QTest::addColumn<QUrl>("url");
	QTest::addColumn<bool>("succeed");
	QTest::addColumn<int>("status");
	QTest::addColumn<QtRestClient::RestObject*>("result");
	QTest::addColumn<bool>("except");

	QTest::newRow("get") << QUrl("http://localhost:3000/posts/1")
						 << true
						 << 200
						 << (QtRestClient::RestObject*)JphPost::createDefault(this)
						 << false;

	QTest::newRow("notFound") << QUrl("http://localhost:3000/posts/baum")
							  << false
							  << 404
							  << new QtRestClient::RestObject(this)
							  << false;

	QTest::newRow("serExcept") << QUrl("http://localhost:3000/posts")
							   << false
							   << 0
							   << new QtRestClient::RestObject(this)
							   << true;
}

void RestReplyTest::testGenericReplyWrapping()
{
	QFETCH(QUrl, url);
	QFETCH(bool, succeed);
	QFETCH(int, status);
	QFETCH(QtRestClient::RestObject*, result);
	QFETCH(bool, except);

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	bool called = false;

	auto reply = new QtRestClient::GenericRestReply<JphPost>(nam->get(request), client);
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *rep, int code, JphPost *data){
		called = true;
		QVERIFY(succeed);
		QVERIFY(!except);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QVERIFY(QtRestClient::RestObject::equals(data, result));
		data->deleteLater();
	});
	reply->onFailed([&](QtRestClient::RestReply *rep, int code, QtRestClient::RestObject *data){
		called = true;
		QVERIFY(!succeed);
		QVERIFY(!except);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QVERIFY(QtRestClient::RestObject::equals(data, result));
		data->deleteLater();
	});
	reply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		called = true;
		QFAIL(qUtf8Printable(error));
	});
	reply->onSerializeException([&](QtRestClient::RestReply *rep, QtRestClient::SerializerException &){
		called = true;
		QVERIFY(!succeed);
		QVERIFY(except);
		QCOMPARE(rep, reply);
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteSpy.wait());
	QVERIFY(called);
}

void RestReplyTest::testGenericListReplyWrapping_data()
{
	QTest::addColumn<QUrl>("url");
	QTest::addColumn<bool>("succeed");
	QTest::addColumn<int>("status");
	QTest::addColumn<int>("count");
	QTest::addColumn<QtRestClient::RestObject*>("firstResult");
	QTest::addColumn<bool>("except");

	QTest::newRow("get") << QUrl("http://localhost:3000/posts")
						 << true
						 << 200
						 << 100
						 << (QtRestClient::RestObject*)JphPost::createDefault(this)
						 << false;

	QTest::newRow("notFound") << QUrl("http://localhost:3000/postses")
							  << false
							  << 404
							  << 0
							  << new QtRestClient::RestObject(this)
							  << false;

	QTest::newRow("serExcept") << QUrl("http://localhost:3000/posts/1")
							   << false
							   << 0
							   << 0
							   << new QtRestClient::RestObject(this)
							   << true;
}

void RestReplyTest::testGenericListReplyWrapping()
{
	QFETCH(QUrl, url);
	QFETCH(bool, succeed);
	QFETCH(int, status);
	QFETCH(int, count);
	QFETCH(QtRestClient::RestObject*, firstResult);
	QFETCH(bool, except);

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	bool called = false;

	auto reply = new QtRestClient::GenericRestReply<QList<JphPost>>(nam->get(request), client);
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *rep, int code, QList<JphPost*> data){
		called = true;
		QVERIFY(succeed);
		QVERIFY(!except);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QCOMPARE(data.size(), count);
		QVERIFY(QtRestClient::RestObject::equals(data.first(), firstResult));
		qDeleteAll(data);
	});
	reply->onFailed([&](QtRestClient::RestReply *rep, int code, QtRestClient::RestObject *data){
		called = true;
		QVERIFY(!succeed);
		QVERIFY(!except);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QVERIFY(QtRestClient::RestObject::equals(data, firstResult));
		data->deleteLater();
	});
	reply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		called = true;
		QFAIL(qUtf8Printable(error));
	});
	reply->onSerializeException([&](QtRestClient::RestReply *rep, QtRestClient::SerializerException &){
		called = true;
		QVERIFY(!succeed);
		QVERIFY(except);
		QCOMPARE(rep, reply);
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteSpy.wait());
	QVERIFY(called);
}

void RestReplyTest::testGenericPagingReplyWrapping_data()
{
	QTest::addColumn<QUrl>("url");
	QTest::addColumn<bool>("succeed");
	QTest::addColumn<int>("status");
	QTest::addColumn<int>("offset");
	QTest::addColumn<int>("limit");
	QTest::addColumn<int>("total");
	QTest::addColumn<QtRestClient::RestObject*>("firstResult");
	QTest::addColumn<bool>("except");

	QTest::newRow("get") << QUrl("http://localhost:3000/pages/0")
						 << true
						 << 200
						 << 0
						 << 10
						 << 100
						 << (QtRestClient::RestObject*)JphPost::createDefault(this)
						 << false;

	QTest::newRow("notFound") << QUrl("http://localhost:3000/pageses")
							  << false
							  << 404
							  << 0
							  << 0
							  << 0
							  << new QtRestClient::RestObject(this)
							  << false;

	QTest::newRow("serExcept") << QUrl("http://localhost:3000/posts/1")
							   << false
							   << 0
							   << 0
							   << 0
							   << 0
							   << new QtRestClient::RestObject(this)
							   << true;
}

void RestReplyTest::testGenericPagingReplyWrapping()
{
	QFETCH(QUrl, url);
	QFETCH(bool, succeed);
	QFETCH(int, status);
	QFETCH(int, offset);
	QFETCH(int, limit);
	QFETCH(int, total);
	QFETCH(QtRestClient::RestObject*, firstResult);
	QFETCH(bool, except);

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	bool called = false;

	auto reply = new QtRestClient::GenericRestReply<QtRestClient::Paging<JphPost>>(nam->get(request), client);
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *rep, int code, QtRestClient::Paging<JphPost> data){
		called = true;
		QVERIFY(succeed);
		QVERIFY(!except);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QVERIFY(data.isValid());
		QCOMPARE(data.offset(), offset);
		QCOMPARE(data.limit(), limit);
		QCOMPARE(data.total(), total);
		QVERIFY(QtRestClient::RestObject::equals(data.items().first(), firstResult));
		data.deleteAllItems();
	});
	reply->onFailed([&](QtRestClient::RestReply *rep, int code, QtRestClient::RestObject *data){
		called = true;
		QVERIFY(!succeed);
		QVERIFY(!except);
		QCOMPARE(rep, reply);
		QCOMPARE(code, status);
		QVERIFY(QtRestClient::RestObject::equals(data, firstResult));
		data->deleteLater();
	});
	reply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		called = true;
		QFAIL(qUtf8Printable(error));
	});
	reply->onSerializeException([&](QtRestClient::RestReply *rep, QtRestClient::SerializerException &){
		called = true;
		QVERIFY(!succeed);
		QVERIFY(except);
		QCOMPARE(rep, reply);
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteSpy.wait());
	QVERIFY(called);
}

void RestReplyTest::testPagingNext()
{
	QNetworkRequest request(QStringLiteral("http://localhost:3000/pages/0"));
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	bool called = false;
	QtRestClient::Paging<JphPost> firstPaging;

	auto reply = new QtRestClient::GenericRestReply<QtRestClient::Paging<JphPost>>(nam->get(request), client);
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *rep, int code, QtRestClient::Paging<JphPost> data){
		called = true;
		QCOMPARE(rep, reply);
		QCOMPARE(code, 200);
		QVERIFY(data.isValid());
		QVERIFY(!data.hasPrevious());
		QVERIFY(data.hasNext());
		QCOMPARE(data.offset(), 0);
		QCOMPARE(data.limit(), 10);
		QCOMPARE(data.total(), 100);
		data.deleteAllItems();
		firstPaging = data;
	});
	reply->onFailed([&](QtRestClient::RestReply *, int, QtRestClient::RestObject *data){
		called = true;
		QFAIL("onFailed");
		data->deleteLater();
	});
	reply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		called = true;
		QFAIL(qUtf8Printable(error));
	});
	reply->onSerializeException([&](QtRestClient::RestReply *, QtRestClient::SerializerException &e){
		called = true;
		QFAIL(e.what());
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteSpy.wait());
	QVERIFY(called);

	QVERIFY(firstPaging.isValid());
	called = false;
	auto nextReply = firstPaging.next();
	nextReply->enableAutoDelete();
	nextReply->onSucceeded([&](QtRestClient::RestReply *rep, int code, QtRestClient::Paging<JphPost> data){
		called = true;
		QCOMPARE(rep, nextReply);
		QCOMPARE(code, 200);
		QVERIFY(data.isValid());
		QCOMPARE(data.offset(), 10);
		QCOMPARE(data.limit(), 20);
		QCOMPARE(data.total(), 100);
		data.deleteAllItems();
	});
	nextReply->onFailed([&](QtRestClient::RestReply *, int, QtRestClient::RestObject *data){
		called = true;
		QFAIL("onFailed");
		data->deleteLater();
	});
	nextReply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		called = true;
		QFAIL(qUtf8Printable(error));
	});
	nextReply->onSerializeException([&](QtRestClient::RestReply *, QtRestClient::SerializerException &e){
		called = true;
		QFAIL(e.what());
	});

	QSignalSpy deleteNextSpy(nextReply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteNextSpy.wait());
	QVERIFY(called);
}

void RestReplyTest::testPagingPrevious()
{
	QNetworkRequest request(QStringLiteral("http://localhost:3000/pages/9"));
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	bool called = false;
	QtRestClient::Paging<JphPost> lastPaging;

	auto reply = new QtRestClient::GenericRestReply<QtRestClient::Paging<JphPost>>(nam->get(request), client);
	reply->enableAutoDelete();
	reply->onSucceeded([&](QtRestClient::RestReply *rep, int code, QtRestClient::Paging<JphPost> data){
		called = true;
		QCOMPARE(rep, reply);
		QCOMPARE(code, 200);
		QVERIFY(data.isValid());
		QVERIFY(data.hasPrevious());
		QVERIFY(!data.hasNext());
		QCOMPARE(data.offset(), 90);
		QCOMPARE(data.limit(), 100);
		QCOMPARE(data.total(), 100);
		data.deleteAllItems();
		lastPaging = data;
	});
	reply->onFailed([&](QtRestClient::RestReply *, int, QtRestClient::RestObject *data){
		called = true;
		QFAIL("onFailed");
		data->deleteLater();
	});
	reply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		called = true;
		QFAIL(qUtf8Printable(error));
	});
	reply->onSerializeException([&](QtRestClient::RestReply *, QtRestClient::SerializerException &e){
		called = true;
		QFAIL(e.what());
	});

	QSignalSpy deleteSpy(reply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deleteSpy.wait());
	QVERIFY(called);

	QVERIFY(lastPaging.isValid());
	called = false;
	auto prevReply = lastPaging.previous();
	prevReply->enableAutoDelete();
	prevReply->onSucceeded([&](QtRestClient::RestReply *rep, int code, QtRestClient::Paging<JphPost> data){
		called = true;
		QCOMPARE(rep, prevReply);
		QCOMPARE(code, 200);
		QVERIFY(data.isValid());
		QCOMPARE(data.offset(), 80);
		QCOMPARE(data.limit(), 90);
		QCOMPARE(data.total(), 100);
		data.deleteAllItems();
	});
	prevReply->onFailed([&](QtRestClient::RestReply *, int, QtRestClient::RestObject *data){
		called = true;
		QFAIL("onFailed");
		data->deleteLater();
	});
	prevReply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		called = true;
		QFAIL(qUtf8Printable(error));
	});
	prevReply->onSerializeException([&](QtRestClient::RestReply *, QtRestClient::SerializerException &e){
		called = true;
		QFAIL(e.what());
	});

	QSignalSpy deletePrevSpy(prevReply, &QtRestClient::RestReply::destroyed);
	QVERIFY(deletePrevSpy.wait());
	QVERIFY(called);
}

void RestReplyTest::testPagingIterate()
{
	QNetworkRequest request(QStringLiteral("http://localhost:3000/pages/0"));
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	auto count = 0;
	auto reply = new QtRestClient::GenericRestReply<QtRestClient::Paging<JphPost>>(nam->get(request), client);
	reply->enableAutoDelete();
	reply->iterate([&](QtRestClient::Paging<JphPost> *paging, JphPost *data, int index){
		auto ok = false;
		[&](){
			QCOMPARE(index, count++);
			QVERIFY(paging->isValid());
			QCOMPARE(data->id, count);//validating the id is enough
			ok = true;
		}();
		if(!ok || count == 100)
			emit testPagingIterate_unlock();
		data->deleteLater();
		return ok;
	});
	reply->onFailed([&](QtRestClient::RestReply *, int, QtRestClient::RestObject *data){
		count = 110;
		QFAIL("onFailed");
		data->deleteLater();
	});
	reply->onError([&](QtRestClient::RestReply *, QString error, int, QtRestClient::RestReply::ErrorType){
		count = 120;
		QFAIL(qUtf8Printable(error));
	});
	reply->onSerializeException([&](QtRestClient::RestReply *, QtRestClient::SerializerException &e){
		count = 130;
		QFAIL(e.what());
	});

	QSignalSpy completedSpy(this, &RestReplyTest::testPagingIterate_unlock);
	QVERIFY(completedSpy.wait(15000));
	QCOMPARE(count, 100);

	QCoreApplication::processEvents();//to ensure all deleteLaters have been called!
}

QTEST_MAIN(RestReplyTest)

#include "tst_restreply.moc"
