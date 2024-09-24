#pragma once

#include <Arduino.h>
#include <memory>

namespace fs {
class FS;
}

class ArduinoMultiWiFi {
public:
    class CredentialSource {
    public:
        virtual ~CredentialSource() = default;

        /**
         * Save the credentials of the currently connected network. On restart a connection to those credentials is
         * attempted first.
         * @see getLastSuccessfulCredential
         * @param ssid
         * @param password
         */
        virtual void saveCurrentCredentials(const String& ssid, const String& password) {
        }

        /**
         * Retrieve saved credentials
         * @see saveCurrentCredentials
         * @param ssid
         * @param password
         * @return
         */
        virtual bool getLastSuccessfulCredential(String& ssid, String& password) {
            return false;
        }

        virtual bool hasCredential(String ssid) = 0;
        virtual String getPassword(String ssid) = 0;
    };

    static const int ConnectTimeout = 4000;

    explicit ArduinoMultiWiFi(std::unique_ptr<CredentialSource> credentialSource);

    virtual ~ArduinoMultiWiFi();

    static void setHostname(const char* hostname);

    static void disablePowersafe();

    void loop();

private:
    enum class State {
        Disabled,
        StartScan,
        Scanning,
        Connecting,
        Connected,
        RecoveryAP,
    };

    void loopScan();

    void startRecoveryAP();

    std::unique_ptr<CredentialSource> m_credentialSource;
    State m_state{State::Disabled};
    unsigned long m_connectTimeout{0};
    String m_currentSsid;
    String m_currentPassword;
};
