#include <WiFi.h>
#include <WebServer.h>
#include <stdint.h>
#include <secrets.h>


#define ROOMBA_TX_PIN 17          /* Roomba RX */
#define ROOMBA_RX_PIN 16          /* Roomba TX*/
#define ROOMBA_BAUD   19200       


#define OI_START  128             
#define OI_BAUD   129
#define OI_SAFE   131             
#define OI_FULL   132             
#define OI_DRIVE  137             

#define VELOCITY_GO      250      
#define VELOCITY_TURN    200      
#define RADIUS_STRAIGHT  32768    
#define RADIUS_SPIN_CCW  1        
#define RADIUS_SPIN_CW   (-1)     

static WebServer server(80);
static void r_write(uint8_t b)
{
    Serial2.write(b);
    delayMicroseconds(100);
}

/* DRIVE: velocity in mm/s (-500..500), radius in mm (-2000..2000) */
static void roomba_drive(int16_t velocity, int16_t radius)
{
    r_write(OI_DRIVE);
    r_write((uint8_t)(velocity >> 8));   /* high byte first (big-endian) */
    r_write((uint8_t)(velocity & 0xFF));
    r_write((uint8_t)(radius >> 8));
    r_write((uint8_t)(radius & 0xFF));
}

static void roomba_init(void)
{
    Serial2.begin(ROOMBA_BAUD, SERIAL_8N1, ROOMBA_RX_PIN, ROOMBA_TX_PIN);
    delay(1000);                 /* let the Roomba settle after power-up */

    r_write(OI_START);           /* 128: enter passive mode */
    delay(20);
    r_write(OI_SAFE);            /* 131: safe mode, motors enabled */
    delay(20);
    
}

static void roomba_stop(void)   { roomba_drive(0, 0); }
static void roomba_go(void)     { roomba_drive(VELOCITY_GO, RADIUS_STRAIGHT); }
static void roomba_back(void)   { roomba_drive(-VELOCITY_GO, RADIUS_STRAIGHT); }
static void roomba_left(void)   { roomba_drive(VELOCITY_TURN, RADIUS_SPIN_CCW); }
static void roomba_right(void)  { roomba_drive(VELOCITY_TURN, RADIUS_SPIN_CW); }

static const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Roomba Control</title>
<style>
  body { font-family: sans-serif; text-align: center; background: #111; color: #eee; }
  .grid { display: grid; grid-template-columns: repeat(3, 110px); gap: 10px;
          justify-content: center; margin-top: 30px; }
  button { width: 110px; height: 70px; font-size: 20px; border: none;
           border-radius: 10px; background: #6d4aff; color: #fff; }
  button:active { background: #fff; color: #111; }
  .wide { grid-column: span 3; background: #d33; }
</style>
</head>
<body>
<h2>Roomba 612 Control</h2>
<div class="grid">
  <span></span>
  <button onclick="send('go')">GO</button>
  <span></span>
  <button onclick="send('left')">LEFT</button>
  <button onclick="send('stop')" style="background:#555">STOP</button>
  <button onclick="send('right')">RIGHT</button>
  <button onclick="send('back')" class="wide">BACK</button>
</div>
<script>
function send(cmd) {
  fetch('/cmd?cmd=' + cmd);
}
</script>
</body>
</html>
)rawliteral";

static void handleRoot(void)
{
    server.send_P(200, "text/html", PAGE_HTML);
}

static void handleCmd(void)
{
    String cmd = server.arg("cmd");

    if      (cmd == "go")    roomba_go();
    else if (cmd == "back")  roomba_back();
    else if (cmd == "left")  roomba_left();
    else if (cmd == "right") roomba_right();
    else                     roomba_stop();     /* anything unknown = stop */

    server.send(200, "text/plain", "ok");
}

/* ---------------- Arduino entry points ---------------- */

void setup(void)
{
    Serial.begin(115200);                    /* debug console */
    roomba_init();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Open http://");
    Serial.println(WiFi.localIP());          /* browse to this address */

    server.on("/", handleRoot);
    server.on("/cmd", HTTP_GET, handleCmd);
    server.begin();
}

void loop(void)
{
    server.handleClient();
}