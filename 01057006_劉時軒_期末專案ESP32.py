from umqtt.simple import MQTTClient
from machine import Pin, UART
import utime, xtools, gc

# 定義按鈕
button = Pin(0, Pin.IN, Pin.PULL_UP)

# 定義UART
com = UART(2, 9600, tx=17, rx=16)
com.init(9600)

# 連接網路
xtools.connect_wifi_led()

# MQTT 客戶端
client = MQTTClient (
    client_id = xtools.get_id(),
    server = "broker.hivemq.com",
    ssl = False,
)

# 初始化密碼
password = ""

# 回撥函數 (接收到MQTT傳送訊息 數字猜測內容) 
def sub_cb(topic, msg):
    global password
    mode = msg.decode()
    if 'A' in mode or 'B' in mode:
        return
    if "SET" in mode:
        password = mode.replace("SET", "").strip()  # 去掉 'SET' 並去除兩端的空格
        print("設定密碼: ", password)
        return
    print("收到訊息: ", mode)
    # 透過 UART 傳送訊息給 8051
    com.write(mode)
    com.write(b'\r\n')

client.set_callback(sub_cb)   # 指定回撥函數來接收訊息
client.connect()              # 連線

topic = "es" + "/project/" + "01057006"
print(topic)
client.subscribe(topic)      # 訂閱主題

while True:
    gc.collect()
    client.check_msg()
    if com.any() > 0: # 收到8051透過UART傳遞的訊息
        hint = com.readline()
        hint_str = hint.decode('utf-8')
        if "CK" in hint_str:
            guess = hint_str.replace("CK", "").strip()  # 去掉 'SET' 並去除兩端的空格
            print("猜測密碼: ", guess)
            print("正確密碼: ", password)
            a = sum(1 for s, g in zip(password, guess) if s == g)
            b = sum(min(password.count(x), guess.count(x)) for x in set(guess)) - a
            result = f"{a}A{b}B"
            print(result)
            com.write(result.encode('utf-8'))
            com.write(b'\r\n')
        else:
           client.publish(topic, hint_str) # 發送訊息給MQTT
           print("提示: " + hint_str)
    utime.sleep(2)
    # client.publish(topic, "ON") # 發送訊息
    # utime.sleep(2)