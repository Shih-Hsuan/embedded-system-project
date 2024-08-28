# 嵌入式期末專案
## 主題: 物聯網密碼鎖與音樂播放系統設計
### 功能介紹
* 8051設定密碼 MQTT猜密碼
> 使用者可以透過MQTT的手機APP輸入四個數字(不重複)，去跟8051上設定的密碼做比對，8051會回傳XAXB，表示是否正確猜中該密碼，沒猜中可以繼續猜，若猜中了將解鎖音樂撥放功能，使用者可以透MQTT APP 輸入音名、高低音、節拍，來撥放音樂
* MQTT設定密碼 8051猜密碼
> 使用者可以透過8051輸入四個數字(不重複)，去跟MQTT上設定的密碼做比對，ESP32會回傳XAXB，表示是否正確猜中該密碼，沒猜中可以繼續猜，若猜中了將解鎖音樂撥放功能，使用者可以透MQTT APP 輸入音名、高低音、節拍，來撥放音樂

### 實作方式
> 8051 與 ESP32 使用 UART 溝通，並通過MQTT傳遞訊息
### 實際操作方式
* 可以利用8051上的矩陣鍵盤控制
  * 0~9: 輸入的密碼
  * 10: 設定密碼
  * 11: 比對猜測密碼
  * 12: 傳送猜測密碼
* ESP32 & MQTT
  * 輸入密碼: 0~9(四位數字)
  * 接收訊息: XAXB
  * 出題者: 字串格式 ex: SET1234
  * 設定音樂: 字串格式 ex: PLAY[頻率][高低][拍子]
* MQTT TOPIC:
    * es/project/01057006
### 接線定義
* 8051:
    * 八個七段顯示器: P0
    * 矩陣鍵盤: P1
    * 段鎖存: P2^2
    * 位鎖存: P2^3
    * 繼電器: P2^7
* ESP32 <-> 8051
    * GPIO17 <-> RX
    * GPIO16 <-> TX
    * GPIO15 <-> SPK
    * GND <-> GND
