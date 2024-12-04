// Khai báo thư viện
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>

// Thông tin WiFi
#define WIFI_SSID "King"
#define WIFI_PASSWORD "denbongsuakhoemanh"

// Thông tin Firebase
#define FIREBASE_HOST "https://smart-garden-f8433-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "cdA89JyTaGOqvfcIia7V5l4g5WTc6WBiqIkWjTrO"

// Khai báo các chân GPIO
int den = 16;
int tuoi = 4;
int quat = 17;
int mai = 5;
int LM393_PIN = 34;  // Chân đọc tín hiệu từ LM393

// Cấu hình cảm biến DHT11
#define DHTPIN 27 // Chân nối cảm biến DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Khởi tạo BH1750
BH1750 lightMeter;

// Biến lưu dữ liệu cảm biến
int nhietDo = 0, doAmKK = 0, doSang = 0; // Sử dụng kiểu int cho các biến
int doAmDat = 0;

// Khởi tạo Firebase
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
    // Cấu hình các chân thiết bị đầu ra
    pinMode(den, OUTPUT); digitalWrite(den, LOW);
    pinMode(tuoi, OUTPUT); digitalWrite(tuoi, LOW);
    pinMode(quat, OUTPUT); digitalWrite(quat, LOW);
    pinMode(mai, OUTPUT); digitalWrite(mai, LOW);
    pinMode(LM393_PIN, INPUT);  // Đầu vào kỹ thuật số cho LM393

    // Khởi tạo các cảm biến
    Serial.begin(115200);
    dht.begin();
    Wire.begin();
    lightMeter.begin();

    // Kết nối WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected.");

    // Cấu hình Firebase
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;

    // Kết nối Firebase
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void loop() {
    // Đọc và gửi dữ liệu từ các cảm biến lên Firebase
    docDuLieuDHT11();
    docDuLieuBH1750();
    docDuLieuLM393();

    // Nhận lệnh điều khiển thiết bị từ Firebase
    nhanLenhTuFirebase("/Thiet bi/Den", den);
    nhanLenhTuFirebase("/Thiet bi/Tuoi", tuoi);
    nhanLenhTuFirebase("/Thiet bi/Quat", quat);
    nhanLenhTuFirebase("/Thiet bi/Mai che", mai);

    delay(1000);  // Chờ 1 giây trước khi lặp lại
}

// Đọc dữ liệu từ cảm biến DHT11
void docDuLieuDHT11() {
    nhietDo = (int)dht.readTemperature();  // Chuyển đổi giá trị nhiệt độ thành số nguyên
    doAmKK = (int)dht.readHumidity();  // Chuyển đổi giá trị độ ẩm thành số nguyên

    if (!isnan(nhietDo) && !isnan(doAmKK)) {
        Firebase.setInt(firebaseData, "/Nhiet do", nhietDo);  // Gửi dữ liệu nhiệt độ lên Firebase
        Firebase.setInt(firebaseData, "/Do am", doAmKK);  // Gửi dữ liệu độ ẩm lên Firebase
        Serial.printf("Nhiet do: %d\n", nhietDo); 
        Serial.printf("Do am: %d\n", doAmKK);  // In ra giá trị nhiệt độ và độ ẩm
    } else {
        Serial.println("Loi khi doc DHT11");
    }
}

// Đọc dữ liệu từ cảm biến BH1750
void docDuLieuBH1750() {
    doSang = (int)lightMeter.readLightLevel();  // Chuyển đổi giá trị độ sáng thành số nguyên
    Firebase.setInt(firebaseData, "/Do sang", doSang);  // Gửi dữ liệu độ sáng lên Firebase
    Serial.printf("Do sang: %d lux\n", doSang);  // In ra giá trị độ sáng
}

// Đọc dữ liệu từ cảm biến độ ẩm đất (LM393)
void docDuLieuLM393() {
    int sensorValue = analogRead(LM393_PIN);  // Đọc giá trị analog từ cảm biến độ ẩm đất
    doAmDat = 100 - (sensorValue / 4095.0) * 100;  // Tính toán độ ẩm đất
    Firebase.setInt(firebaseData, "/Do am dat", doAmDat);  // Gửi dữ liệu độ ẩm đất lên Firebase
    Serial.printf("Do am dat: %d\n", doAmDat);  // In ra giá trị độ ẩm đất
}

// Nhận lệnh từ Firebase và điều khiển thiết bị
void nhanLenhTuFirebase(const String& path, int pin) {
    if (Firebase.getInt(firebaseData, path)) {
        int trangThai = firebaseData.intData();
        digitalWrite(pin, trangThai ? HIGH : LOW);
        Serial.printf("%s: %s\n", path.c_str(), trangThai ? "Bat" : "Tat");
    } else {
        Serial.printf("Loi doc %s: %s\n", path.c_str(), firebaseData.errorReason().c_str());
    }
}
