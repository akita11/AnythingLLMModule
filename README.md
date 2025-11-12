# AnythingModuleLLM

<img src="https://github.com/akita11/AnythingModuleLLM/blob/main/AnythingModuleLLM_StampS3A_1.jpg" width="240px">

<img src="https://github.com/akita11/AnythingModuleLLM/blob/main/AnythingModuleLLM_StampS3A_2.jpg" width="240px">

M5Stack Coreシリーズに接続し、他のPC等で動作しているAIモデル（大規模言語モデル社(LLM)）を、M5Stackから使用できるようにするModuleです。M5Stackとは、M5Stack社のLLM Moduleと同等の接続となるため、ソフトウエア的にはLLM Moduleと同様に扱うことができます。他のPCのLLMへはUSBやWiFiで接続します。M5StampS3Aに必要なソフトウエアを書き込んで使用してください。StampS3Aの取り付け方法が2種類あります。

## StampS3Aを直接とりつけ

[StampS3A](https://www.switch-science.com/products/10377)をリフローはんだ付け等で直接基板に実装します。M5Stack接続用のMbusコネクタもリフローはんだ付け等で基板に実装してください。また必要に応じて、Module Frameをとりつけてください。


## StampS3Aをコネクタを介してとりつけ

<img src="https://github.com/akita11/AnythingModuleLLM/blob/main/AnythingModuleLLM_parts.jpg" width="240px">

[2.54mmピンヘッダ実装済みのStampS3A](https://www.switch-science.com/products/10734)と、6pと9p（または8p）のピンソケット、およびMbusコネクタまたは2.54mmの2列x15ピンの低プロファイルのピンヘッダを用意します。

<img src="https://github.com/akita11/AnythingModuleLLM/blob/main/AnythingModuleLLM_build1.jpg" width="240px">

まず基板のこちら面に、6pと9p（または8p）のピンソケットをはんだ付けします。8pソケットを使う場合は、この写真のように1列分を開けた位置にとりつけてください。

<img src="https://github.com/akita11/AnythingModuleLLM/blob/main/AnythingModuleLLM_build2.jpg" width="240px">

Mbusコネクタをはんだ付け、または2.54mmの2列x15ピンの低プロファイルのピンヘッダをこの写真のようにはんだ付けします。M5Stack本体に接続される側（この写真では下側）の端子の先端が、基板面から5.5mmの位置になるようにしてください。

最後に[2.54mmピンヘッダ実装済みのStampS3A](https://www.switch-science.com/products/10734)を取り付けて完成です。


## Author

Junichi Akita (@akita11) / akita@ifdl.jp
Ogawa3427
