// ============================================================
//  Bengala Inteligente — Arduino Uno + HC-SR04 + Buzzer
//  v2 — corrige eco fantasma e travamento de zona
// ============================================================

#define NOTA_GRAVE        400
#define NOTA_MEDIA        800
#define NOTA_AGUDA       1200
#define NOTA_CRITICA     2000

#define PINO_TRIGGER        8
#define PINO_ECHO           7
#define PINO_BUZZER        10

#define DISTANCIA_LONGE       100
#define DISTANCIA_MEDIA        60
#define DISTANCIA_PERTO        40
#define DISTANCIA_MUITO_PERTO  15
#define DISTANCIA_CRITICA       6

#define NUM_LEITURAS        3
#define TIMEOUT_ECHO    30000UL
#define DIST_INVALIDA    9999.0

#define BIP_MEDIA         600
#define BIP_PERTO         300
#define BIP_MUITO_PERTO   120
#define BIP_CRITICO        50

unsigned long ultimoBip      = 0;
bool          bipLigado      = false;
float         distanciaAtual = DIST_INVALIDA;
int           zonaAnterior   = -1;          // ✅ NOVO

// ============================================================
void setup() {
  pinMode(PINO_BUZZER,  OUTPUT);
  pinMode(PINO_TRIGGER, OUTPUT);
  pinMode(PINO_ECHO,    INPUT);
  digitalWrite(PINO_TRIGGER, LOW);
  Serial.begin(9600);
  Serial.println(F("Bengala Inteligente v2 iniciada."));
}

// ============================================================
void loop() {
  distanciaAtual = medirDistanciaFiltrada();

  Serial.print(F("Distancia: "));
  if (distanciaAtual >= DIST_INVALIDA) {
    Serial.println(F("--- SEM LEITURA ---"));
  } else {
    Serial.print(distanciaAtual, 1);
    Serial.println(F(" cm"));
  }

  gerarAlerta(distanciaAtual);
  // ✅ delay removido daqui — o delay(65)*3 dentro do filtro já cobre ~200ms
}

// ============================================================
float medirDistancia() {
  digitalWrite(PINO_TRIGGER, LOW);
  delayMicroseconds(4);
  digitalWrite(PINO_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(PINO_TRIGGER, LOW);

  unsigned long duracao = pulseIn(PINO_ECHO, HIGH, TIMEOUT_ECHO);

  if (duracao == 0) return DIST_INVALIDA;

  float dist = (duracao * 0.0343f) / 2.0f;
  if (dist < 2.0f || dist > 400.0f) return DIST_INVALIDA;

  return dist;
}

// ============================================================
float medirDistanciaFiltrada() {
  float soma    = 0.0f;
  int   validas = 0;

  for (int i = 0; i < NUM_LEITURAS; i++) {
    float leitura = medirDistancia();
    if (leitura < DIST_INVALIDA) {
      soma += leitura;
      validas++;
    }
    delay(65); // ✅ CORRIGIDO: era delayMicroseconds(500) — HC-SR04 precisa de ≥60ms
  }

  return (validas > 0) ? (soma / validas) : DIST_INVALIDA;
}

// ============================================================
void gerarAlerta(float dist) {

  // --- Determina zona atual ---
  int zonaAtual;
  if      (dist >= DIST_INVALIDA)            zonaAtual = -1; // sem leitura
  else if (dist > DISTANCIA_LONGE)           zonaAtual =  0; // longe
  else if (dist > DISTANCIA_MEDIA)           zonaAtual =  1;
  else if (dist > DISTANCIA_PERTO)           zonaAtual =  2;
  else if (dist > DISTANCIA_MUITO_PERTO)     zonaAtual =  3;
  else                                        zonaAtual =  4; // crítica

  // ✅ Troca de zona — reseta estado para não ficar preso
  if (zonaAtual != zonaAnterior) {
    noTone(PINO_BUZZER);
    bipLigado    = false;
    ultimoBip    = 0;
    zonaAnterior = zonaAtual;
  }

  // Sem leitura ou longe: silêncio
  if (zonaAtual <= 0) {
    noTone(PINO_BUZZER);
    bipLigado = false;
    return;
  }

  // Zona crítica: tom contínuo
  if (zonaAtual == 4) {
    tone(PINO_BUZZER, NOTA_CRITICA);
    bipLigado = true;
    return;
  }

  // Zonas intermediárias: bip intermitente
  int           frequencia;
  unsigned long intervalo;

  if      (zonaAtual == 1) { frequencia = NOTA_GRAVE; intervalo = BIP_MEDIA;       }
  else if (zonaAtual == 2) { frequencia = NOTA_MEDIA; intervalo = BIP_PERTO;       }
  else                     { frequencia = NOTA_AGUDA; intervalo = BIP_MUITO_PERTO; }

  unsigned long agora = millis();
  if (agora - ultimoBip >= intervalo) {
    ultimoBip = agora;
    if (bipLigado) {
      noTone(PINO_BUZZER);
      bipLigado = false;
    } else {
      tone(PINO_BUZZER, frequencia);
      bipLigado = true;
    }
  }
}