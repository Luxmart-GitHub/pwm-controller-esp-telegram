// This source contains specifically the Telegram
// binding of the PWM controller.
// All generic code is  moved into the header files below.

#include "pwm_controller_config.h"
#include "api/pwm_controller_commands.h"
#include "common/pwm_controller_esp.h"
#include "common/pwm_controller_user.h"

// Universal Telegram Bot Library written by Brian Lough:
// https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

class TelegramUserControl : public UserControl
{
  WiFiClientSecure client;
  UniversalTelegramBot bot;

  // Checks for new messages every 1 second.
  int botRequestDelay = 1000;

  unsigned long lastTimeBotRan;

  char state = LED_LOW;

public :

  TelegramUserControl() : bot(BOT_TOKEN, client)
  {
    lastTimeBotRan = millis();
#ifdef ESP8266
    client.setInsecure();
#endif
  }

  virtual void iterate()
  {
    if (millis() > lastTimeBotRan + botRequestDelay)
    {
      DBGLOG("iterate");

      digitalWrite(LED_BUILTIN, state);
      state ^= LED_HIGH;

      while (1)
      {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        if (!numNewMessages) break;

        handleNewMessages(numNewMessages);
      }

      lastTimeBotRan = millis();
    }
  }

  // Handle new receive messages
  void handleNewMessages(int numNewMessages)
  {
    for (int i = 0; i < numNewMessages; i++)
    {
      String chat_id = String(bot.messages[i].chat_id);

      // Check that the chat ID is ours.
      if (chat_id != CHAT_ID){
        // Unauthorized user.
        continue;
      }

      // Print the received message
      auto text = bot.messages[i].text;
      if (text == "/start")
      {
        auto from_name = bot.messages[i].from_name;
        String welcome = "Welcome to PWM Controller, " + from_name + "!\n\n";
        welcome += "The following commands are supported:\n\n";
        for (int j = 0; j < sizeof(descriptions) / sizeof(descriptions[0]); j++)
        {
          auto d = descriptions[j];
          welcome += "/";
          welcome += char(d.command);
          welcome += " : ";
          welcome += d.message;
          welcome += "\n";
        }
        bot.sendMessage(chat_id, welcome, "");
      }
      else if (text == "/p")
      {
        DBGLOG("Rebooting...");
#if 0
        bot.sendMessage(chat_id, "Device will now reboot!", "");
        ESP.restart();
#endif
      }
      else if (text.length() > 1)
      {
        char command = text[1];
        char* extraArg = nullptr;
        if (text.length() > 2)
          extraArg = &text[2];

        char returnedValue;  
        auto result = executeCommand(command, extraArg, &returnedValue);
        String error("Error: command \"");
        error += command;
        error += "\" ";
        switch (result)
        {
        case NoReturnedValue :
          break;
        case ReturnsValue :
          bot.sendMessage(chat_id, String("") + static_cast<unsigned int>(
            *reinterpret_cast<unsigned char*>(&returnedValue)), "");
          break;
        case ErrorCommandNotRecognized :
          bot.sendMessage(chat_id, error + "not recognized", "");
          break;
        case ErrorCommandArgumentMissing :
          bot.sendMessage(chat_id, error + "not executed, because a required argument is missing");
          break;
        case ErrorCommandTimedout :
          bot.sendMessage(chat_id, error + "could not execute (or return a value) for too long");
          break;
        default :
          bot.sendMessage(chat_id, error + "has unexpected result");
        }
      }
    }
  }
};

static UserControl* userControl;

void pwm_controller_setup()
{
  userControl = new TelegramUserControl();
}

void pwm_controller_loop()
{
  userControl->iterate();
}