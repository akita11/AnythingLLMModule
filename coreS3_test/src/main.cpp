/*
 * SPDX-FileCopyrightText: 2025 @Ogawa3427
 * SPDX-License-Identifier: MIT
 * Special thanks to M5Stack
 * Special thanks to ミクミンPさん https://gist.github.com/ksasao/37425d3463013221e7fd0f9ae5ab1c62
 */

#include <Arduino.h>
#include <M5Unified.h>
#include <M5ModuleLLM.h>

M5ModuleLLM module_llm;
String llm_work_id;

void setup()
{
  M5.begin();
  M5.Display.setTextSize(1);
  M5.Display.setTextScroll(true);
  M5.Lcd.setTextFont(&fonts::efontCN_12_b);
  Serial.begin(115200);

  Serial2.begin(115200, SERIAL_8N1, 18, 17);
  module_llm.begin(&Serial2);

  M5.Display.println("module_llm.checkConnection()");
  Serial.println("module_llm.checkConnection()");
  while (1)
  {
    int result = module_llm.checkConnection();
    Serial.print("Check ModuleLLM connection result:");
    Serial.println(result);
    M5.Display.print("Check ModuleLLM connection result:");
    M5.Display.println(result);
    if (result == 0)
    {
      M5.Display.setTextColor(TFT_GREEN);
      M5.Display.println("Check ModuleLLM connection success");
      Serial.println("Check ModuleLLM connection success");
      break;
    }
    else
    {
      M5.Display.println("Check ModuleLLM connection failed");
      Serial.println("Check ModuleLLM connection failed");
      delay(500);
    }
  }

  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.println("module_llm.sys.reset()");
  Serial.println("module_llm.sys.reset()");
  module_llm.sys.reset();

  Serial.println("module_llm.llm.setup()  [getting work_id]");
  M5.Display.println("module_llm.llm.setup()  [getting work_id]");
  m5_module_llm::ApiLlmSetupConfig_t config;
  // お使いのPCなどに入っているものに変更してください
  config.model = "qwen3:8b";
  llm_work_id = module_llm.llm.setup(config);
  if (llm_work_id.length() == 0)
  {
    while (1)
    {
      M5.Display.setTextColor(TFT_RED);
      M5.Display.println("LLM setup failed");
      Serial.println("LLM setup failed");
      delay(1000);
    }
  }
  M5.Display.setTextColor(TFT_GREEN);
  M5.Display.println("LLM setup success");
  Serial.println("LLM setup success");
  M5.Display.setTextColor(TFT_WHITE);
  Serial.print("LLM setup result(Work ID):");
  Serial.println(llm_work_id);
  M5.Display.print("LLM setup result(Work ID):");
  M5.Display.setTextColor(TFT_YELLOW);
  M5.Display.println(llm_work_id);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.clear();
  M5.Display.setCursor(0, 0);
  M5.Display.println("Ready");
  Serial.println("Ready");
}

void loop()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readString();

    M5.Display.setTextColor(TFT_CYAN);
    M5.Display.print("[ME]: ");
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println(input);
    Serial.print("[ME]: ");
    Serial.println(input);
    M5.Display.setTextColor(TFT_MAGENTA);
    M5.Display.print("[LLM]: ");
    M5.Display.setTextColor(TFT_WHITE);
    Serial.print("[LLM]: ");

    module_llm.llm.inferenceAndWaitResult(llm_work_id, input.c_str(), [](String &result)
                                          {
          M5.Display.printf("%s", result.c_str());
          Serial.printf("%s", result.c_str()); });
    Serial.println();
    M5.Display.println();
  }

  delay(500);
}