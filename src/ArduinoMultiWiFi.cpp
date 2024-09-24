#include "ArduinoMultiWiFi.h"

#include <WiFi.h>
#include <esp_wifi.h>

ArduinoMultiWiFi::ArduinoMultiWiFi(std::unique_ptr<CredentialSource> credentialSource) :
        m_credentialSource{std::move(credentialSource)},
        m_state{State::StartScan} {
    loop(); // Call loop once to start scanning

    String ssid;
    String password;
    if (m_credentialSource->getLastSuccessfulCredential(ssid, password)) {
        WiFi.begin(ssid.c_str(), password.c_str());
    }
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
}

ArduinoMultiWiFi::~ArduinoMultiWiFi() {
    WiFi.disconnect(true, true);
}

void ArduinoMultiWiFi::setHostname(const char* hostname) {
    WiFiClass::setHostname(hostname);
}

void ArduinoMultiWiFi::disablePowersafe() {
    WiFi.setSleep(false);
}

void ArduinoMultiWiFi::loop() {
    switch (m_state) {
        case State::StartScan:
            Serial.println("Start WiFi scan");
            if (WiFi.scanNetworks(true) != WIFI_SCAN_RUNNING) {
                WiFi.disconnect();
                Serial.println("Failed to start WiFi scan");
                return;
            }
            m_state = State::Scanning;
            break;
        case State::Scanning:
            loopScan();
            break;
        case State::Connecting:
            if (WiFi.isConnected()) {
                Serial.println("WiFi connected");
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                m_credentialSource->saveCurrentCredentials(m_currentSsid, m_currentPassword);
                m_connectTimeout = 0;
                m_state = State::Connected;
            } else if (millis() > m_connectTimeout) {
                Serial.println("WiFi connect timed out");
                WiFi.disconnect();
                m_state = State::StartScan;
            }
            break;
        case State::Connected:
            if (!WiFi.isConnected()) {
                Serial.println("WiFi disconnected");
                m_state = State::StartScan;
            }
            break;
        case State::RecoveryAP:
        case State::Disabled:
            break;
    }
}

void ArduinoMultiWiFi::loopScan() {
    int scanResult = WiFi.scanComplete();
    if (scanResult < 0) {
        return;
    }
    m_state = State::StartScan;
    Serial.printf("Found %i wifi networks\n", WiFi.scanComplete());

    if (WiFi.isConnected()) {
        Serial.println("Already connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        WiFi.scanDelete();
        m_state = State::Connected;
        return;
    }

    if (WiFi.scanComplete() > 0) {
        for (int i = 0; i < scanResult; i++) {
            String ssid = WiFi.SSID(i);
            Serial.printf("SSID: %s\n", ssid.c_str());
            if (m_credentialSource->hasCredential(ssid)) {
                Serial.printf("Connecting to %s\n", ssid.c_str());
                m_currentSsid = ssid;
                m_currentPassword = m_credentialSource->getPassword(ssid);
                WiFi.begin(m_currentSsid.c_str(), m_currentPassword.c_str());
                m_connectTimeout = millis() + ConnectTimeout;
                m_state = State::Connecting;
                break;
            }
        }
    }
    WiFi.scanDelete();
}

void ArduinoMultiWiFi::startRecoveryAP() {
    Serial.println("Starting recovery WiFi AP");
    WiFi.disconnect();
    WiFi.softAP(WiFiClass::getHostname(), "00000000");
    WiFi.softAPConfig({192, 168, 120, 1}, {192, 168, 120, 1}, {255, 255, 255, 0}, {192, 168, 120, 10});
    m_state = State::RecoveryAP;
}
