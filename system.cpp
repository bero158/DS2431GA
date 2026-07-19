#include <arduino.h>
#include "settings.h"
#include "system.h"
namespace systemFNC {
    void restart(String reason) {
        delay(5000);
        ESP.restart();
        delay(5000);
    }
}