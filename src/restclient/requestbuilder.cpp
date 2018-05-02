#include "requestbuilder.h"
#include "restreply_p.h"
#include "restclass.h"

#include <QtCore/QBuffer>
#include <QtCore/QJsonDocument>
using namespace QtRestClient;

namespace QtRestClient {

struct RequestBuilderPrivate : public QSharedData
{
	static const QByteArray ContentType;
	static const QByteArray ContentTypeJson;
	static const QByteArray ContentTypeUrlEncoded;

	QNetworkAccessManager *nam;

	QUrl base;
	QVersionNumber version;
	QString user;
	QString pass;
	QStringList path;
	bool trailingSlash = false;
	QUrlQuery query;
	QString fragment;
	HeaderHash headers;
	QHash<QNetworkRequest::Attribute, QVariant> attributes;
	QSslConfiguration sslConfig;
	QByteArray body;
	QByteArray verb;
	QUrlQuery postQuery;

	inline RequestBuilderPrivate(QUrl baseUrl = QUrl(), QNetworkAccessManager *nam = nullptr) :
		QSharedData(),
		nam(nam),
		base(baseUrl),
		user(baseUrl.userName()),
		pass(baseUrl.password()),
		query(baseUrl.query()),
		fragment(baseUrl.fragment()),
		sslConfig(QSslConfiguration::defaultConfiguration()),
		verb(RestClass::GetVerb)
	{}

	inline RequestBuilderPrivate(const RequestBuilderPrivate &other) = default;
};

const QByteArray RequestBuilderPrivate::ContentType = "Content-Type";
const QByteArray RequestBuilderPrivate::ContentTypeJson = "application/json";
const QByteArray RequestBuilderPrivate::ContentTypeUrlEncoded = "application/x-www-form-urlencoded";

}

RequestBuilder::RequestBuilder(const QUrl &baseUrl, QNetworkAccessManager *nam) :
	d(new RequestBuilderPrivate(baseUrl, nam))
{}

RequestBuilder::RequestBuilder(const RequestBuilder &other) = default;

RequestBuilder::RequestBuilder(RequestBuilder &&other) = default;

RequestBuilder &RequestBuilder::operator=(const RequestBuilder &other) = default;

RequestBuilder &RequestBuilder::operator=(RequestBuilder &&other) = default;

RequestBuilder::~RequestBuilder() = default;

RequestBuilder &RequestBuilder::setVersion(const QVersionNumber &version)
{
	d->version = version;
	return *this;
}

RequestBuilder &RequestBuilder::setCredentials(const QString &user, const QString &password)
{
	d->user = user;
	d->pass = password;
	return *this;
}

RequestBuilder &RequestBuilder::addHeader(const QByteArray &name, const QByteArray &value)
{
	d->headers.insert(name, value);
	return *this;
}

RequestBuilder &RequestBuilder::addHeaders(const HeaderHash &headers)
{
	for(auto it = headers.constBegin(); it != headers.constEnd(); it++)
		d->headers.insert(it.key(), it.value());
	return *this;
}

RequestBuilder &RequestBuilder::updateFromRelativeUrl(const QUrl &url, bool mergeQuery, bool keepFragment)
{
	auto cUrl = buildUrl();
	d->base = cUrl.resolved(url);
	if(d->base.host() != cUrl.host()) {
		qWarning() << "URL host changed from"
				   << cUrl.host()
				   << "to"
				   << d->base.host();
	}
	//clear all the rest
	d->version = QVersionNumber();
	d->user.clear();
	d->pass.clear();
	d->path.clear();
	if(mergeQuery) {
		QUrlQuery query(url.query());
		for(const auto &item : query.queryItems())
			d->query.addQueryItem(item.first, item.second);
	} else
		d->query = QUrlQuery(url.query());
	if(!keepFragment)
		d->fragment.clear();
	return *this;
}

RequestBuilder &RequestBuilder::addParameter(const QString &name, const QString &value)
{
	d->query.addQueryItem(name, value);
	return *this;
}

RequestBuilder &RequestBuilder::addParameters(const QUrlQuery &parameters)
{
	for(const auto &param : parameters.queryItems())
		d->query.addQueryItem(param.first, param.second);
	return *this;
}

RequestBuilder &RequestBuilder::setFragment(const QString &fragment)
{
	d->fragment = fragment;
	return *this;
}

RequestBuilder &RequestBuilder::addPath(const QString &pathSegment)
{
	d->path.append(pathSegment.split(QLatin1Char('/'), QString::SkipEmptyParts));
	return *this;
}

RequestBuilder &RequestBuilder::addPath(const QStringList &pathSegment)
{
	d->path.append(pathSegment);
	return *this;
}

RequestBuilder &RequestBuilder::trailingSlash(bool enable)
{
	d->trailingSlash = enable;
	return *this;
}

RequestBuilder &RequestBuilder::setAttribute(QNetworkRequest::Attribute attribute, const QVariant &value)
{
	d->attributes.insert(attribute, value);
	return *this;
}

RequestBuilder &RequestBuilder::setAttributes(const QHash<QNetworkRequest::Attribute, QVariant> &attributes)
{
	for(auto it = attributes.constBegin(); it != attributes.constEnd(); it++)
		d->attributes.insert(it.key(), it.value());
	return *this;
}

RequestBuilder &RequestBuilder::setSslConfig(const QSslConfiguration &sslConfig)
{
	d->sslConfig = sslConfig;
	return *this;
}

RequestBuilder &RequestBuilder::setBody(const QByteArray &body, const QByteArray &contentType)
{
	d->body = body;
	d->headers.insert(RequestBuilderPrivate::ContentType, contentType);
	return *this;
}

RequestBuilder &RequestBuilder::setBody(const QJsonObject &body)
{
	d->body = QJsonDocument(body).toJson(QJsonDocument::Compact);
	d->headers.insert(RequestBuilderPrivate::ContentType, RequestBuilderPrivate::ContentTypeJson);
	return *this;
}

RequestBuilder &RequestBuilder::setBody(const QJsonArray &body)
{
	d->body = QJsonDocument(body).toJson(QJsonDocument::Compact);
	d->headers.insert(RequestBuilderPrivate::ContentType, RequestBuilderPrivate::ContentTypeJson);
	return *this;
}

RequestBuilder &RequestBuilder::setVerb(const QByteArray &verb)
{
	d->verb = verb;
	return *this;
}

RequestBuilder &RequestBuilder::addPostParameter(const QString &name, const QString &value, bool setContentAndVerb)
{
	d->postQuery.addQueryItem(name, value);
	if(setContentAndVerb) {
		d->headers.insert(RequestBuilderPrivate::ContentType, RequestBuilderPrivate::ContentTypeUrlEncoded);
		d->verb = RestClass::PostVerb;
	}
	return *this;
}

RequestBuilder &RequestBuilder::addPostParameters(const QUrlQuery &parameters, bool setContentAndVerb)
{
	for(const auto &param : parameters.queryItems())
		d->postQuery.addQueryItem(param.first, param.second);
	if(setContentAndVerb) {
		d->headers.insert(RequestBuilderPrivate::ContentType, RequestBuilderPrivate::ContentTypeUrlEncoded);
		d->verb = RestClass::PostVerb;
	}
	return *this;
}

QUrl RequestBuilder::buildUrl() const
{
	auto url = d->base;

	auto pathList = url.path().split(QLatin1Char('/'), QString::SkipEmptyParts);
	if(!d->version.isNull())
		pathList.append(QLatin1Char('v') + d->version.normalized().toString());
	pathList.append(d->path);
	url.setPath(QLatin1Char('/') + pathList.join(QLatin1Char('/')) + (d->trailingSlash ? QStringLiteral("/") : QString()));

	if(!d->user.isNull())
		url.setUserName(d->user);
	if(!d->pass.isNull())
		url.setPassword(d->pass);
	if(!d->query.isEmpty())
		url.setQuery(d->query);
	if(!d->fragment.isNull())
		url.setFragment(d->fragment);

	return url;
}

QNetworkRequest RequestBuilder::build() const
{
	QNetworkRequest request(buildUrl());
	for(auto it = d->headers.constBegin(); it != d->headers.constEnd(); it++)
		request.setRawHeader(it.key(), it.value());
	for(auto it = d->attributes.constBegin(); it != d->attributes.constEnd(); it++)
		request.setAttribute(it.key(), it.value());
	request.setSslConfiguration(d->sslConfig);
	return request;
}

QNetworkReply *RequestBuilder::send() const
{
	auto request = build();

	QBuffer *buffer = nullptr;
	if(!d->body.isEmpty()) {
		buffer = new QBuffer();
		buffer->setData(d->body);
		buffer->open(QIODevice::ReadOnly);
	} else if(d->headers.value(RequestBuilderPrivate::ContentType) == RequestBuilderPrivate::ContentTypeUrlEncoded &&
			  !d->postQuery.isEmpty()) {
		buffer = new QBuffer();
		buffer->setData(d->postQuery.query().toUtf8());
		buffer->open(QIODevice::ReadOnly);
	}

	return RestReplyPrivate::compatSend(d->nam, request, d->verb, buffer);
}
