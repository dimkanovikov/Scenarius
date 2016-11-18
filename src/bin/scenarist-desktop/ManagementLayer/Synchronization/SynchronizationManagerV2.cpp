/*
* Copyright (C) 2016 Alexey Polushkin, armijo38@yandex.ru
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* Full license: http://dimkanovikov.pro/license/GPLv3
*/

#include "SynchronizationManagerV2.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <3rd_party/Widgets/QLightBoxWidget/qlightboxprogress.h>
#include <3rd_party/Helpers/PasswordStorage.h>

#include <WebLoader.h>

#include <QXmlStreamReader>
#include <QEventLoop>
#include <QTimer>
#include <QDesktopServices>
#include <QDateTime>

using ManagementLayer::SynchronizationManagerV2;
using DataStorageLayer::StorageFacade;
using DataStorageLayer::SettingsStorage;

namespace {
	/**
	 * @brief Список URL адресов, по которым осуществляются запросы
	 */
	/** @{ */
	const QUrl URL_SIGNUP = QUrl("https://kitscenarist.ru/api/account/register/");
	const QUrl URL_RESTORE = QUrl("https://kitscenarist.ru/api/account/restore/");
	const QUrl URL_LOGIN = QUrl("https://kitscenarist.ru/api/account/login/");
	const QUrl URL_LOGOUT = QUrl("https://kitscenarist.ru/api/account/logout/");
	const QUrl URL_UPDATE = QUrl("https://kitscenarist.ru/api/account/update/");
	const QUrl URL_SUBSCRIBE_STATE = QUrl("https://kitscenarist.ru/api/account/subscribe/state/");
	//
	const QUrl URL_PROJECTS = QUrl("https://kitscenarist.ru/api/projects/");
	const QUrl URL_CREATE_PROJECT = QUrl("https://kitscenarist.ru/api/projects/create/");
	const QUrl URL_UPDATE_PROJECT = QUrl("https://kitscenarist.ru/api/projects/edit/");
	const QUrl URL_REMOVE_PROJECT = QUrl("https://kitscenarist.ru/api/projects/remove/");
	const QUrl URL_CREATE_PROJECT_SUBSCRIPTION = QUrl("https://kitscenarist.ru/api/projects/share/create/");
	const QUrl URL_REMOVE_PROJECT_SUBSCRIPTION = QUrl("https://kitscenarist.ru/api/projects/share/remove/");
	/** @} */

	/**
	 * @brief Список названий параметров для запросов
	 */
	/** @{ */
	const QString KEY_EMAIL = "email";
	const QString KEY_LOGIN = "login";
	const QString KEY_USERNAME = "username";
	const QString KEY_PASSWORD = "password";
	const QString KEY_NEW_PASSWORD = "new_password";
	const QString KEY_SESSION_KEY = "session_key";
	const QString KEY_ROLE = "role";
	//
	const QString KEY_PROJECT_ID = "project_id";
	const QString KEY_PROJECT_NAME = "project_name";
	/** @} */

	/**
	 * @brief Список кодов ошибок и соответствующих им описаний
	 */
	/** @{ */
	const int UNKNOWN_ERROR_CODE = 100;
	const QString UNKNOWN_ERROR_STRING = QObject::tr("Unknown error");

	const int SESSION_KEY_NOT_FOUND_CODE = 101;
	const QString SESSION_KEY_NOT_FOUND_STRING = QObject::tr("Session key not found");
	/** @} */
}

namespace {
	/**
	 * @brief Преобразовать дату из гггг.мм.дд чч:мм:сс в дд.мм.гггг
	 */
	QString dateTransform(const QString &_date)
	{
		return QDateTime::fromString(_date, "yyyy-MM-dd hh:mm:ss").toString("dd.MM.yyyy");
	}
}

SynchronizationManagerV2::SynchronizationManagerV2(QObject* _parent, QWidget* _parentView) :
	QObject(_parent),
	m_view(_parentView),
	m_isSubscriptionActive(false),
	m_loader(new WebLoader(this))
{
	initConnections();
}

bool SynchronizationManagerV2::isSubscriptionActive() const
{
	return m_isSubscriptionActive;
}

void SynchronizationManagerV2::autoLogin()
{
	//
	// Получим параметры из хранилища
	//
	const QString email =
			StorageFacade::settingsStorage()->value(
				"application/email",
				SettingsStorage::ApplicationSettings);
	const QString password =
			PasswordStorage::load(
				StorageFacade::settingsStorage()->value(
					"application/password",
					SettingsStorage::ApplicationSettings),
				email
				);

	//
	// Если они не пусты, авторизуемся
	//
	if (!email.isEmpty() && !password.isEmpty()) {
		login(email, password);
	}
}

void SynchronizationManagerV2::login(const QString &_email, const QString &_password)
{

	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_LOGIN, _email);
	m_loader->addRequestAttribute(KEY_PASSWORD, _password);
	QByteArray response = m_loader->loadSync(URL_LOGIN);

	//
	// Считываем результат авторизации
	//
	QXmlStreamReader responseReader(response);
	//
	// Успешно ли завершилась авторизация
	//
	if(!isOperationSucceed(responseReader)) {
		return;
	}

	QString userName;
	QString date;

	bool isActiveFind = false;
	//
	// Найдем наш ключ сессии, имя пользователя, информацию о подписке
	//
	m_sessionKey.clear();
	while (!responseReader.atEnd()) {
		responseReader.readNext();
		if (responseReader.name().toString() == "session_key") {
			responseReader.readNext();
			m_sessionKey = responseReader.text().toString();
			responseReader.readNext();
		} else if (responseReader.name().toString() == "user_name") {
			responseReader.readNext();
			userName = responseReader.text().toString();
			responseReader.readNext();
		} else if (responseReader.name().toString() == "subscribe_is_active") {
			isActiveFind = true;
			responseReader.readNext();
			m_isSubscriptionActive = responseReader.text().toString() == "true";
			responseReader.readNext();
		} else if (responseReader.name().toString() == "subscribe_end") {
			responseReader.readNext();
			date = responseReader.text().toString();
			responseReader.readNext();
		}
	}

	if (!isActiveFind || (isActiveFind && m_isSubscriptionActive && date.isEmpty())) {
		handleError(tr("Got wrong result from server"), 404);
		m_sessionKey.clear();
		return;
	}

	//
	// Не нашли ключ сессии
	//
	if (m_sessionKey.isEmpty()) {
		handleError(SESSION_KEY_NOT_FOUND_STRING, SESSION_KEY_NOT_FOUND_CODE);
		return;
	}

	//
	// Если авторизация успешна, сохраним информацию о пользователе
	//
	StorageFacade::settingsStorage()->setValue(
				"application/email",
				_email,
				SettingsStorage::ApplicationSettings);
	StorageFacade::settingsStorage()->setValue(
				"application/password",
				PasswordStorage::save(_password, _email),
				SettingsStorage::ApplicationSettings);

	//
	// Запомним email
	//
	m_userEmail = _email;

	emit subscriptionInfoLoaded(m_isSubscriptionActive, dateTransform(date));
	emit loginAccepted(userName, m_userEmail);
}

void SynchronizationManagerV2::signUp(const QString& _email, const QString& _password)
{
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_EMAIL, _email);
	m_loader->addRequestAttribute(KEY_PASSWORD, _password);
	QByteArray response = m_loader->loadSync(URL_SIGNUP);

	//
	// Считываем результат авторизации
	//
	QXmlStreamReader responseReader(response);
	//
	// Успешно ли завершилась авторизация
	//
	if(!isOperationSucceed(responseReader)) {
		return;
	}

	//
	// Найдем наш ключ сессии
	//
	m_sessionKey.clear();
	while (!responseReader.atEnd()) {
		responseReader.readNext();
		if (responseReader.name().toString() == "session_key") {
			responseReader.readNext();
			m_sessionKey = responseReader.text().toString();
			responseReader.readNext();
			break;
		}
	}

	//
	// Не нашли ключ сессии
	//
	if (m_sessionKey.isEmpty()) {
		handleError(SESSION_KEY_NOT_FOUND_STRING, SESSION_KEY_NOT_FOUND_CODE);
		return;
	}

	//
	// Если авторизация прошла
	//
	emit signUpFinished();
}

void SynchronizationManagerV2::verify(const QString& _code)
{
	//
	// FIXME: Поменять в рабочей версии
	//
	QEventLoop event;
	QTimer::singleShot(2000, &event, SLOT(quit()));
	event.exec();

	bool success = false;
	if (_code == "11111") {
		success = true;
	} else {
		handleError("Wrong code", 505);
	}

	if (success) {
		emit verified();
	}
}

void SynchronizationManagerV2::restorePassword(const QString &_email)
{
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_EMAIL, _email);
	QByteArray response = m_loader->loadSync(URL_RESTORE);

	//
	// Считываем результат
	//
	QXmlStreamReader responseReader(response);
	//
	// Успешно ли завершилась операция
	//
	if(!isOperationSucceed(responseReader)) {
		return;
	}

	//
	// Найдем статус
	//
	while (!responseReader.atEnd()) {
		responseReader.readNext();
		if (responseReader.name().toString() == "send_mail_result") {
			responseReader.readNext();
			QString status = responseReader.text().toString();
			responseReader.readNext();
			if (status != "success") {
				handleError(status, UNKNOWN_ERROR_CODE);
				return;
			}
			break;
		}
	}

	//
	// Если успешно отправили письмо
	//
	emit passwordRestored();
}

void SynchronizationManagerV2::logout()
{
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	QByteArray response = m_loader->loadSync(URL_LOGOUT);

	m_sessionKey.clear();
	m_userEmail.clear();

	//
	// Удаляем сохраненные значения, если они были
	//
	StorageFacade::settingsStorage()->setValue(
				"application/email",
				QString::null,
				SettingsStorage::ApplicationSettings);
	StorageFacade::settingsStorage()->setValue(
				"application/password",
				QString::null,
				SettingsStorage::ApplicationSettings);
	StorageFacade::settingsStorage()->setValue(
				"application/remote-projects",
				QString::null,
				SettingsStorage::ApplicationSettings);

	//
	// Если деавторизация прошла
	//
	emit logoutFinished();
}

void SynchronizationManagerV2::renewSubscription(unsigned _duration,
												 unsigned _type)
{
	QDesktopServices::openUrl(QUrl(QString("http://kitscenarist.ru/api/account/subscribe/?"
										   "user=%1&month=%2&payment_type=%3").
								   arg(m_userEmail).arg(_duration).
								   arg(_type == 0 ? "AC" : "PC")));
}

void SynchronizationManagerV2::changeUserName(const QString &_newUserName)
{
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	m_loader->addRequestAttribute(KEY_USERNAME, _newUserName);
	QByteArray response = m_loader->loadSync(URL_UPDATE);

	//
	// Считываем результат авторизации
	//
	QXmlStreamReader responseReader(response);

	if (!isOperationSucceed(responseReader)) {
		return;
	}
	emit userNameChanged();
}

void SynchronizationManagerV2::loadSubscriptionInfo()
{
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	QByteArray response = m_loader->loadSync(URL_SUBSCRIBE_STATE);

	//
	// Считываем результат авторизации
	//
	QXmlStreamReader responseReader(response);
	if (!isOperationSucceed(responseReader)) {
		return;
	}

	//
	// Распарсим результат
	//
	bool isActive;
	QString date;

	bool isActiveFind = false;
	while (!responseReader.atEnd()) {
		responseReader.readNext();
		if (responseReader.name().toString() == "subscribe_is_active") {
			isActiveFind = true;
			responseReader.readNext();
			m_isSubscriptionActive = responseReader.text().toString() == "true";
			responseReader.readNext();
		} else if (responseReader.name().toString() == "subscribe_end") {
			responseReader.readNext();
			date = responseReader.text().toString();
			responseReader.readNext();
		}
	}

	if (!isActiveFind || (isActiveFind && m_isSubscriptionActive && date.isEmpty())) {
		handleError(tr("Got wrong result from server"), 404);
		return;
	}

	emit subscriptionInfoLoaded(m_isSubscriptionActive, dateTransform(date));
}

void SynchronizationManagerV2::changePassword(const QString& _password,
											  const QString& _newPassword)
{
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	m_loader->addRequestAttribute(KEY_PASSWORD, _password);
	m_loader->addRequestAttribute(KEY_NEW_PASSWORD, _newPassword);
	QByteArray response = m_loader->loadSync(URL_UPDATE);

	//
	// Считываем результат авторизации
	//
	QXmlStreamReader responseReader(response);

	if (!isOperationSucceed(responseReader)) {
		return;
	}
	emit passwordChanged();
}

void SynchronizationManagerV2::loadProjects()
{
	//
	// Получаем список проектов
	//
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	const QByteArray response = m_loader->loadSync(URL_PROJECTS);

	//
	// Считываем результат
	//
	QXmlStreamReader responseReader(response);
	if (!isOperationSucceed(responseReader)) {
		return;
	}

	//
	// Сохраняем список проектов для работы в автономном режиме
	//
	StorageFacade::settingsStorage()->setValue("application/remote-projects",
		response.toBase64(), SettingsStorage::ApplicationSettings);

	//
	// Уведомляем о получении проектов
	//
	emit projectsLoaded(response);
}

int SynchronizationManagerV2::createProject(const QString& _projectName)
{
	const int INVALID_PROJECT_ID = -1;

	//
	// Создаём новый проект
	//
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	m_loader->addRequestAttribute(KEY_PROJECT_NAME, _projectName);
	const QByteArray response = m_loader->loadSync(URL_CREATE_PROJECT);

	//
	// Считываем результат
	//
	QXmlStreamReader responseReader(response);
	if (!isOperationSucceed(responseReader)) {
		return INVALID_PROJECT_ID;
	}

	//
	// Считываем идентификатор проекта
	//
	int newProjectId = INVALID_PROJECT_ID;
	while (!responseReader.atEnd()) {
		responseReader.readNext();
		if (responseReader.name().toString() == "project") {
			newProjectId = responseReader.attributes().value("id").toInt();
			break;
		}
	}

	if (newProjectId == INVALID_PROJECT_ID) {
		handleError(tr("Got wrong result from server"), 404);
		return INVALID_PROJECT_ID;
	}

	//
	// Если проект успешно добавился перечитаем список проектов
	//
	loadProjects();

	return newProjectId;
}

void SynchronizationManagerV2::updateProjectName(int _projectId, const QString& _newProjectName)
{
	//
	// Обновляем проект
	//
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	m_loader->addRequestAttribute(KEY_PROJECT_ID, _projectId);
	m_loader->addRequestAttribute(KEY_PROJECT_NAME, _newProjectName);
	const QByteArray response = m_loader->loadSync(URL_UPDATE_PROJECT);

	//
	// Считываем результат
	//
	QXmlStreamReader responseReader(response);
	if (!isOperationSucceed(responseReader)) {
		return;
	}

	//
	// Если проект успешно обновился перечитаем список проектов
	//
	loadProjects();
}

void SynchronizationManagerV2::removeProject(int _projectId)
{
	//
	// Удаляем проект
	//
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	m_loader->addRequestAttribute(KEY_PROJECT_ID, _projectId);
	const QByteArray response = m_loader->loadSync(URL_REMOVE_PROJECT);

	//
	// Считываем результат
	//
	QXmlStreamReader responseReader(response);
	if (!isOperationSucceed(responseReader)) {
		return;
	}

	//
	// Если проект успешно удалён перечитаем список проектов
	//
	loadProjects();
}

void SynchronizationManagerV2::shareProject(int _projectId, const QString& _userEmail, int _role)
{
	const QString userRole = _role == 0 ? "redactor" : "commentator";

	//
	// ДОбавляем подписчика в проект
	//
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	m_loader->addRequestAttribute(KEY_PROJECT_ID, _projectId);
	m_loader->addRequestAttribute(KEY_EMAIL, _userEmail);
	m_loader->addRequestAttribute(KEY_ROLE, userRole);
	const QByteArray response = m_loader->loadSync(URL_CREATE_PROJECT_SUBSCRIPTION);

	//
	// Считываем результат
	//
	QXmlStreamReader responseReader(response);
	if (!isOperationSucceed(responseReader)) {
		return;
	}

	//
	// Если подписчик добавлен, перечитаем список проектов
	//
	loadProjects();
}

void SynchronizationManagerV2::unshareProject(int _projectId, const QString& _userEmail)
{
	//
	// Убираем подписчика из проекта
	//
	m_loader->setRequestMethod(WebLoader::Post);
	m_loader->clearRequestAttributes();
	m_loader->addRequestAttribute(KEY_SESSION_KEY, m_sessionKey);
	m_loader->addRequestAttribute(KEY_PROJECT_ID, _projectId);
	m_loader->addRequestAttribute(KEY_EMAIL, _userEmail.trimmed());
	const QByteArray response = m_loader->loadSync(URL_REMOVE_PROJECT_SUBSCRIPTION);

	//
	// Считываем результат
	//
	QXmlStreamReader responseReader(response);
	if (!isOperationSucceed(responseReader)) {
		return;
	}

	//
	// Если подписчик удалён перечитаем список проектов
	//
	loadProjects();
}

bool SynchronizationManagerV2::isOperationSucceed(QXmlStreamReader& _responseReader)
{
	while (!_responseReader.atEnd()) {
		_responseReader.readNext();
		if (_responseReader.name().toString() == "status") {
			//
			// Операция успешна
			//
			if (_responseReader.attributes().value("result").toString() == "true") {
				return true;
			} else {
				//
				// Попытаемся извлечь код ошибки
				//
				if (!_responseReader.attributes().hasAttribute("errorCode")) {
					//
					// Неизвестная ошибка
					//
					handleError(UNKNOWN_ERROR_STRING, UNKNOWN_ERROR_CODE);
					m_sessionKey.clear();
					return false;
				}

				int errorCode = _responseReader.attributes().value("errorCode").toInt();

				//
				// Попытаемся извлечь текст ошибки
				//
				QString errorText = UNKNOWN_ERROR_STRING;
				if (_responseReader.attributes().hasAttribute("error")) {
					errorText = _responseReader.attributes().value("error").toString();
				}

				//
				// Скажем про ошибку
				//
				handleError(errorText, errorCode);
				return false;
			}
		}
	}
	//
	// Ничего не нашли про статус. Скорее всего пропал интернет
	//
	handleError(tr("Can't estabilish network connection."), 0);
	return false;
}

void SynchronizationManagerV2::handleError(const QString &_error, int _code)
{
	emit syncClosedWithError(_code, _error);
}

void SynchronizationManagerV2::initConnections()
{
	connect(this, &SynchronizationManagerV2::loginAccepted, this, &SynchronizationManagerV2::loadProjects);
}
