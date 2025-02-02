/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "settingsmanager.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

#include "constants.h"
#include "cryptosettings.h"
#include "leakdetector.h"
#include "logger.h"

namespace {

Logger logger("SettingsManager");

SettingsManager* s_instance = nullptr;

#ifndef UNIT_TEST
constexpr const char* SETTINGS_ORGANIZATION_NAME = "mozilla";
#else
constexpr const char* SETTINGS_ORGANIZATION_NAME = "mozilla_testing";
#endif

const QSettings::Format MozFormat = QSettings::registerFormat(
    "moz", CryptoSettings::readFile, CryptoSettings::writeFile);

}  // namespace

// static
SettingsManager* SettingsManager::instance() {
  if (!s_instance) {
    s_instance = new SettingsManager(qApp);
  }

  return s_instance;
}

#ifdef UNIT_TEST
// static
void SettingsManager::testCleanup() {
  Q_ASSERT(s_instance);
  delete s_instance;
}
#endif

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent),
      m_settings(MozFormat, QSettings::UserScope, SETTINGS_ORGANIZATION_NAME,
                 Constants::SETTINGS_APP_NAME),
      m_settingsConnector(this, &m_settings) {
  MZ_COUNT_CTOR(SettingsManager);

  logger.debug() << "Initializing SettingsManager";

  LogHandler::instance()->registerLogSerializer(this);
}

SettingsManager::~SettingsManager() {
  MZ_COUNT_DTOR(SettingsManager);

  logger.debug() << "Destroying SettingsManager";

  LogHandler::instance()->unregisterLogSerializer(this);

#ifdef UNIT_TEST
  hardReset();
#endif

  // Free the memory for everything that was in the map.
  qDeleteAll(m_registeredSettings);
  m_registeredSettings.clear();

  Q_ASSERT(s_instance == this);
  s_instance = nullptr;
}

QString SettingsManager::settingsFileName() { return m_settings.fileName(); }

void SettingsManager::registerSetting(Setting* setting) {
  Q_ASSERT(setting);

  if (m_registeredSettings.contains(setting->key())) {
    return;
  }

  m_registeredSettings.insert(setting->key(), setting);
}

void SettingsManager::reset() {
  logger.debug() << "Clean up the settings";
  foreach (Setting* setting, m_registeredSettings.values()) {
    setting->reset();
  }
}

void SettingsManager::hardReset() {
  logger.debug() << "Hard reset";
  m_settings.clear();

  foreach (Setting* setting, m_registeredSettings.values()) {
    Q_ASSERT(setting);
    setting->changed();
  }
}

void SettingsManager::serializeLogs(
    std::function<void(const QString& name, const QString& logs)>&&
        a_callback) {
  std::function<void(const QString& name, const QString& logs)> callback =
      std::move(a_callback);

  QString buff;
  QTextStream out(&buff);
  foreach (Setting* setting, m_registeredSettings.values()) {
    const auto log = setting->log();
    if (!log.isEmpty()) {
      out << log << Qt::endl;
    }
  }

  callback("Settings", buff);
}

Setting* SettingsManager::createOrGetSetting(
    const QString& key, std::function<QVariant()> defaultValueGetter,
    bool removeWhenReset, bool sensitiveSetting) {
  auto setting = getSetting(key);
  if (setting) {
    Q_ASSERT(defaultValueGetter() == setting->m_defaultValueGetter());
    Q_ASSERT(removeWhenReset == setting->m_removeWhenReset);
    Q_ASSERT(sensitiveSetting == setting->m_sensitiveSetting);

    return setting;
  }

  setting = new Setting(
      this, &m_settingsConnector, key,
      [defaultValueGetter]() { return QVariant(defaultValueGetter()); },
      removeWhenReset, sensitiveSetting);

  registerSetting(setting);
  return setting;
}

SettingGroup* SettingsManager::createSettingGroup(const QString& groupKey,
                                                  bool removeWhenReset,
                                                  bool sensitiveSetting,
                                                  QStringList acceptedKeys) {
  return new SettingGroup(this, &m_settingsConnector, groupKey, removeWhenReset,
                          sensitiveSetting, acceptedKeys);
}
