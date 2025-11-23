// =================================================================================================
// ARQUIVO: ESP32_Placa_Firebase_SIMPLIFICADO.ino
// VERSÃO: FINAL E OTIMIZADA (Removida redefinição de tokenStatusCallback)
// =================================================================================================

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h> 
#include <time.h> 
#include <FirebaseESP32.h> 
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Addons do Firebase
// Este include define a função tokenStatusCallback internamente
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ===================
// Pinos (Modelo AI THINKER - PADRÃO)
// ===================
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// ===================================================
// 🔑 1. CREDENCIAIS FIREBASE (ATUALIZE)
// ===================================================
#define API_KEY "AIzaSyA6D7-8VVoSl6-tx4k4eZS5Pwj63r1XRrQ"
#define DATABASE_URL "https://projeto-96c27-default-rtdb.firebaseio.com/"

// Objetos globais do Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===================================================
// 📶 2. CREDENCIAIS WIFI 
// ===================================================
const char *ssid = "NOVAIS"; 
const char *password = "123456788"; 

// ===================================================
// 🌐 3. CREDENCIAIS PLATERECOGNIZER (ATUALIZE)
// ===================================================
const char* api_url = "https://api.platerecognizer.com/v1/plate-reader/";
// ** ATUALIZE ESTE TOKEN **
const char* token = "Token 3ad74cc4afcb3f256bb107d655d7b899b206389a"; 

// ===================================================
// ESTRUTURA DE DADOS
// ===================================================
struct Deteccao {
    String placa = "";
    float confianca = 0.0;
    bool sucesso = false;
};

// Declarações de funções
Deteccao reconhecerPlaca();
bool enviarRegistroParaFirebase(String placa, float confianca);

// =========================================================================
// ❌ FUNÇÃO DE CALLBACK DO TOKEN (REMOVIDA PARA EVITAR REDEFINIÇÃO)
// A função 'tokenStatusCallback' é fornecida pelo 'TokenHelper.h'. 
// Para que o Firebase saiba que ela existe, basta incluí-la no setup.
// =========================================================================


// =========================================================================
// ⚙️ SETUP
// =========================================================================
void setup() {
    // Desativa Brownout (CRÍTICO para estabilidade)
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
    
    Serial.begin(115200);
    Serial.println("\n========== INICIANDO ESP32-CAM (FINAL) ==========");

    // 1. WIFI
    WiFi.mode(WIFI_STA);
    // Usa as variáveis 'ssid' e 'password'
    WiFi.begin(ssid, password); 
    Serial.println("-------Conectando ao Wi-Fi------");
    Serial.print("Conectando a: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nNão foi possível conectar ao Wi-Fi.");
        ESP.restart();
    } else {
        Serial.println("\nConectado ao Wi-Fi. Endereço IP: ");
        Serial.print("IP atribuído: ");
        Serial.println(WiFi.localIP()); 
        Serial.println("---------------");
    }

    // 2. CÂMERA
    Serial.println("-> Configurando câmera...");
    camera_config_t cam_config;
    
    // Configurações de pinos (Padrão AI-Thinker)
    cam_config.ledc_channel = LEDC_CHANNEL_0; cam_config.ledc_timer = LEDC_TIMER_0; cam_config.pin_d0 = Y2_GPIO_NUM; cam_config.pin_d1 = Y3_GPIO_NUM; cam_config.pin_d2 = Y4_GPIO_NUM; cam_config.pin_d3 = Y5_GPIO_NUM; cam_config.pin_d4 = Y6_GPIO_NUM; cam_config.pin_d5 = Y7_GPIO_NUM; cam_config.pin_d6 = Y8_GPIO_NUM; cam_config.pin_d7 = Y9_GPIO_NUM; cam_config.pin_xclk = XCLK_GPIO_NUM; cam_config.pin_pclk = PCLK_GPIO_NUM; cam_config.pin_vsync = VSYNC_GPIO_NUM; cam_config.pin_href = HREF_GPIO_NUM; cam_config.pin_sccb_sda = SIOD_GPIO_NUM; cam_config.pin_sccb_scl = SIOC_GPIO_NUM; cam_config.pin_pwdn = PWDN_GPIO_NUM; cam_config.pin_reset = RESET_GPIO_NUM;
    
    // Configurações de imagem
    cam_config.xclk_freq_hz = 10000000; 
    cam_config.pixel_format = PIXFORMAT_JPEG;
    cam_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; 
    cam_config.jpeg_quality = 20; 
    
    if (psramFound()) {
        cam_config.fb_count = 2;
        cam_config.fb_location = CAMERA_FB_IN_PSRAM; 
        cam_config.frame_size = FRAMESIZE_SVGA; // 800x600 
    } else {
        cam_config.fb_count = 1;
        cam_config.fb_location = CAMERA_FB_IN_DRAM;
        cam_config.frame_size = FRAMESIZE_QVGA; // 320x240 (se não houver PSRAM)
    }

    esp_err_t err = esp_camera_init(&cam_config);
    if (err != ESP_OK) {
        Serial.printf("ERRO Câmera: 0x%x.\n", err);
        ESP.restart();
    }
    Serial.println("Câmera Pronta!");
    
    // 3. FIREBASE e NTP TIME
    
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    
    // Configura o servidor NTP para obter data e hora (UTC-3 para horário de Brasília)
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); 
    Serial.println("Sincronizando horário...");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Falha ao obter hora. Verifique a conexão NTP.");
        return;
    } else {
        Serial.println(&timeinfo, "Hora atual: %Y-%m-%d %H:%M:%S");
        Serial.println("---------------");
    }

    // Associa o callback do token (A função agora está definida em TokenHelper.h)
    // Se o seu compilador reclamar aqui, comente esta linha e tente novamente.
    // config.token_status_callback = tokenStatusCallback; 

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // ⭐️ Autenticação Anônima para garantir o token (Corrige o erro -127) ⭐️
    if (Firebase.ready() && !auth.token.uid.length()) {
        Serial.println("Tentando autenticação anônima (para obter token válido)...");
        // Passando o endereço (&config) e strings vazias para login anônimo
        if (Firebase.signUp(&config, &auth, "", "")) { 
            Serial.println("SUCESSO: Usuário anônimo autenticado. Token gerado.");
        } else {
            // Usando Serial.printf para imprimir o código de erro numérico
            Serial.printf("ERRO Autenticação Anônima: Código %d\n", config.signer.signupError);
            Serial.println("Verifique se o método de login 'Anônimo' está ativado no Firebase Console.");
        }
    }
    
    Serial.println("Firebase configurado e pronto.");
}

// =========================================================================
// 🔄 LOOP PRINCIPAL: ORQUESTRAÇÃO DE FLUXO
// =========================================================================
void loop() {
    
    // 1. Tenta reconhecer a placa
    Deteccao deteccao = reconhecerPlaca(); 

    // 2. Se a detecção teve sucesso, envia para o Firebase
    if (deteccao.sucesso) {
        
        Serial.println("-> Placa detectada. Enviando para o Firebase (PASSO 2) ---");
        enviarRegistroParaFirebase(deteccao.placa, deteccao.confianca);
        
    } else {
        Serial.println("Nenhuma placa válida detectada ou falha na API.");
    }

    Serial.println("Aguardando 20 segundos para o próximo ciclo...");
    delay(20000); 
}

// =========================================================================
// 📷 FUNÇÃO DE RECONHECIMENTO (Extração de Score Bruto 0.x)
// =========================================================================
Deteccao reconhecerPlaca() {
    Deteccao resultado; 
    
    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();
    
    if (!fb) {
        Serial.println("ERRO: Falha na captura da câmera.");
        return resultado;
    }

    Serial.printf("-> Imagem capturada: %d bytes. Enviando para a API...\n", fb->len);

    WiFiClientSecure client;
    client.setInsecure(); 

    HTTPClient http;
    http.setTimeout(30000); 
    
    if (!http.begin(client, api_url)) {
        Serial.println("ERRO: Falha ao iniciar a conexão HTTP.");
        esp_camera_fb_return(fb);
        return resultado;
    }

    // Montagem do Payload Multipart
    String boundary = "---0123456789ABCDEF"; 
    http.addHeader("Authorization", token);
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    String headerPart = "--" + boundary + "\r\n";
    headerPart += "Content-Disposition: form-data; name=\"upload\"; filename=\"placa.jpg\"\r\n";
    headerPart += "Content-Type: image/jpeg\r\n\r\n";
    String footerPart = "\r\n--" + boundary + "--\r\n";

    int totalLen = headerPart.length() + fb->len + footerPart.length();

    uint8_t *full_payload = (uint8_t *)malloc(totalLen);
    
    if (!full_payload) {
        Serial.println("ERRO: Falha ao alocar memória para o payload.");
        esp_camera_fb_return(fb);
        http.end();
        return resultado;
    }

    // Copia as partes para o buffer único
    int currentPos = 0;
    memcpy(full_payload + currentPos, headerPart.c_str(), headerPart.length());
    currentPos += headerPart.length();
    memcpy(full_payload + currentPos, fb->buf, fb->len);
    currentPos += fb->len;
    memcpy(full_payload + currentPos, footerPart.c_str(), footerPart.length());
    currentPos += footerPart.length();
    
    esp_camera_fb_return(fb); 
    
    // ENVIO DA REQUISIÇÃO
    int httpCode = http.POST(full_payload, totalLen); 
    free(full_payload);

    if (httpCode > 0) {
        String payload = http.getString();
        
        // Processamento da resposta JSON
        int plateIdx = payload.indexOf("\"plate\":\"");
        if (plateIdx > 0) {
            String placa = payload.substring(plateIdx + 9, payload.indexOf("\"", plateIdx + 9));
            placa.toUpperCase();
            
            // ⭐️ EXTRAÇÃO DA CONFIANÇA (SCORE PRINCIPAL DA PLACA) ⭐️
            float confianca = 0.0;
            int candidatesIdx = payload.indexOf("\"candidates\":", plateIdx);
            
            if (candidatesIdx != -1) {
                // Procura pelo último "score" antes de "candidates"
                int scoreKeyIdx = payload.lastIndexOf("\"score\":", candidatesIdx);
                
                if(scoreKeyIdx != -1) {
                    int startValueIdx = scoreKeyIdx + 8; // "score": é 8 caracteres
                    int endValueIdx = payload.indexOf(",", startValueIdx);
                    
                    if (endValueIdx == -1 || endValueIdx > candidatesIdx) {
                        endValueIdx = payload.indexOf("}", startValueIdx);
                    }
                    
                    if (endValueIdx != -1) {
                        String scoreStr = payload.substring(startValueIdx, endValueIdx);
                        scoreStr.trim();
                        scoreStr.replace(",", "."); 
                        confianca = scoreStr.toFloat();
                    }
                }
            }
            // FIM DA EXTRAÇÃO DE SCORE

            resultado.placa = placa; 
            resultado.confianca = confianca; // Armazena o valor bruto (ex: 0.999)
            resultado.sucesso = true;
            Serial.println("-> SUCESSO. Placa: " + resultado.placa);
            Serial.printf("-> Confiança Final: %.3f\n", resultado.confianca); 
            
        } else {
            Serial.println("Nenhuma placa encontrada na resposta da API.");
        }

    } else {
        Serial.printf("-> ERRO HTTP: %s (código: %d)\n", http.errorToString(httpCode).c_str(), httpCode);
    }

    http.end();
    return resultado;
}


// =========================================================================
// 🔥 FUNÇÃO DE ENVIO PARA O FIREBASE (pushJSON para registros)
// =========================================================================
bool enviarRegistroParaFirebase(String placa, float confianca) {
    // Verifica se o Firebase está pronto (se o token foi gerado com sucesso)
    if (!Firebase.ready()) {
        Serial.println("ERRO FIREBASE: Não está pronto para enviar dados. Token inválido.");
        return false;
    }
    
    Serial.println("-> Enviando dados para o Firebase...");

    // 1. Obter Data/Hora
    struct tm timeinfo;
    
    if (!getLocalTime(&timeinfo)) {
        Serial.println("ERRO NTP: A hora pode estar incorreta.");
        return false;
    }
    
    char dataHoraFormatada[25];
    strftime(dataHoraFormatada, sizeof(dataHoraFormatada), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    // 2. Montar JSON
    FirebaseJson json;
    json.set("placa", placa);
    json.set("datahora", String(dataHoraFormatada));
    json.set("confianca", confianca); 
    json.set("id_camera", 1);
    
    // 3. Enviar (pushJSON cria um novo nó único em /registros)
    if (Firebase.pushJSON(fbdo, "/registros", json)) {
        Serial.println("-> SUCESSO: Registro salvo no Firebase!");
        return true;
    } else {
        Serial.println("-> ERRO Firebase: " + fbdo.errorReason());
        Serial.println("Dica: Verifique as Regras de Segurança do Firebase RTDB (write: true).");
        return false;
    }
}