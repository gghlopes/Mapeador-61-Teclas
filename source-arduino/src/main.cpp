#include <Arduino.h>
// Mapeador de matriz de teclado - Arduino Mega
// Conecte os 16 fios do flat nos pinos 22..37 (em ordem).
// Pressione as teclas da MAIS GRAVE para a MAIS AGUDA, uma por vez.
//
// Comandos no Serial Monitor (115200 baud, "Nova linha"):
//   u = desfaz a última tecla   s = pula a tecla atual (deixa sem mapa)
//   p = imprime o mapa agora    r = recomeça do zero

const uint8_t N = 16;
const uint8_t PINS[N] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37};
const int TOTAL_KEYS = 61;
const int FIRST_NOTE = 36;      // C2 = 36 (padrão de 61 teclas: 36..96)

struct KeyMap { int8_t a; int8_t b; bool ab; bool ba; bool mapped; };
KeyMap keys[TOTAL_KEYS];
int current = 0;                // índice da próxima tecla (0..60)

bool m[N][N];

void scanOnce(bool out[N][N]) {
  for (uint8_t i = 0; i < N; i++) pinMode(PINS[i], INPUT_PULLUP);
  for (uint8_t i = 0; i < N; i++) {
    pinMode(PINS[i], OUTPUT);
    digitalWrite(PINS[i], LOW);
    delayMicroseconds(10);
    for (uint8_t j = 0; j < N; j++) {
      out[i][j] = (i != j) && (digitalRead(PINS[j]) == LOW);
    }
    pinMode(PINS[i], INPUT_PULLUP);
  }
}

// Só aceita a leitura quando fica igual em 4 varreduras seguidas (debounce)
void scanStable(bool out[N][N]) {
  bool t[N][N];
  while (true) {
    scanOnce(out);
    bool same = true;
    for (int k = 0; k < 3 && same; k++) {
      delay(5);
      scanOnce(t);
      for (uint8_t i = 0; i < N && same; i++)
        for (uint8_t j = 0; j < N; j++)
          if (t[i][j] != out[i][j]) { same = false; break; }
    }
    if (same) return;
  }
}

// Lista pares não ordenados ativos; retorna quantos
int activePairs(bool mm[N][N], int8_t pa[], int8_t pb[]) {
  int c = 0;
  for (uint8_t i = 0; i < N; i++)
    for (uint8_t j = i + 1; j < N; j++)
      if (mm[i][j] || mm[j][i]) {
        if (c < 20) { pa[c] = i; pb[c] = j; }
        c++;
      }
  return c;
}

int findMapped(int8_t a, int8_t b) {
  for (int k = 0; k < TOTAL_KEYS; k++)
    if (keys[k].mapped &&
        ((keys[k].a == a && keys[k].b == b) || (keys[k].a == b && keys[k].b == a)))
      return k;
  return -1;
}

void printPrompt() {
  if (current < TOTAL_KEYS) {
    Serial.print(F("Pressione a tecla "));
    Serial.print(current + 1);
    Serial.print(F(" de "));
    Serial.print(TOTAL_KEYS);
    Serial.print(F(" (nota MIDI "));
    Serial.print(FIRST_NOTE + current);
    Serial.println(F(")"));
  }
}

void printMap() {
  int diodeDir = 0, diodeBoth = 0, count = 0;
  for (int k = 0; k < TOTAL_KEYS; k++) {
    if (!keys[k].mapped) continue;
    count++;
    if (keys[k].ab && keys[k].ba) diodeBoth++; else diodeDir++;
  }
  Serial.println();
  Serial.println(F("=== INICIO DO MAPA ==="));
  Serial.print(F("TECLAS_MAPEADAS=")); Serial.println(count);
  Serial.print(F("TOTAL=")); Serial.println(TOTAL_KEYS);
  Serial.print(F("PRIMEIRA_NOTA=")); Serial.println(FIRST_NOTE);
  Serial.print(F("PARES_COM_DIODO(sentido unico)=")); Serial.println(diodeDir);
  Serial.print(F("PARES_SEM_DIODO(dois sentidos)=")); Serial.println(diodeBoth);
  Serial.println(F("# tecla,nota,pinoA,pinoB,sentido  (pinos = 1..16 do conector)"));
  Serial.println(F("# sentido: A>B = so conduz com A como saida e B como entrada"));
  Serial.println(F("#          B>A = o inverso;  A=B = conduz nos dois sentidos"));
  for (int k = 0; k < TOTAL_KEYS; k++) {
    Serial.print(k + 1); Serial.print(',');
    Serial.print(FIRST_NOTE + k); Serial.print(',');
    if (!keys[k].mapped) { Serial.println(F("SEM_MAPA")); continue; }
    Serial.print(keys[k].a + 1); Serial.print(',');
    Serial.print(keys[k].b + 1); Serial.print(',');
    if (keys[k].ab && keys[k].ba) Serial.println(F("A=B"));
    else if (keys[k].ab) Serial.println(F("A>B"));
    else Serial.println(F("B>A"));
  }
  Serial.println(F("=== FIM DO MAPA ==="));
  Serial.println(F("Copie tudo entre INICIO e FIM e me envie."));
}

void resetAll() {
  for (int k = 0; k < TOTAL_KEYS; k++) keys[k].mapped = false;
  current = 0;
  Serial.println(F("Recomecando do zero."));
  printPrompt();
}

void waitRelease() {
  while (true) {
    scanStable(m);
    int8_t pa[20], pb[20];
    if (activePairs(m, pa, pb) == 0) return;
  }
}

void handleCommand(char c) {
  if (c == 'p') { printMap(); }
  else if (c == 'r') { resetAll(); }
  else if (c == 's' && current < TOTAL_KEYS) {
    keys[current].mapped = false;
    Serial.print(F("Tecla ")); Serial.print(current + 1); Serial.println(F(" pulada."));
    current++;
    printPrompt();
  }
  else if (c == 'u' && current > 0) {
    current--;
    keys[current].mapped = false;
    Serial.print(F("Desfeito. Refaca a tecla ")); Serial.println(current + 1);
    printPrompt();
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  for (uint8_t i = 0; i < N; i++) pinMode(PINS[i], INPUT_PULLUP);
  delay(500);

  Serial.println(F("=== Mapeador de matriz - Mega ==="));
  Serial.println(F("Comandos: u=desfazer  s=pular  p=imprimir  r=recomecar"));

  // Teste inicial: nenhuma tecla deveria estar fechada em repouso
  scanStable(m);
  int8_t pa[20], pb[20];
  int c = activePairs(m, pa, pb);
  if (c > 0) {
    Serial.println(F("ATENCAO: contatos fechados em repouso (fio trocado, curto ou tecla presa?):"));
    for (int k = 0; k < c && k < 20; k++) {
      Serial.print(F("  pino ")); Serial.print(pa[k] + 1);
      Serial.print(F(" <-> pino ")); Serial.println(pb[k] + 1);
    }
    Serial.println(F("Solte tudo; ele segue assim mesmo, mas confira a ligacao."));
  }
  printPrompt();
}

void loop() {
  while (Serial.available()) handleCommand(Serial.read());
  if (current >= TOTAL_KEYS) return;

  scanStable(m);
  int8_t pa[20], pb[20];
  int c = activePairs(m, pa, pb);
  if (c == 0) return;

  if (c > 1) {
    Serial.println(F("Mais de um contato ao mesmo tempo. Solte tudo e pressione SO uma tecla."));
    waitRelease();
    return;
  }

  int dup = findMapped(pa[0], pb[0]);
  if (dup >= 0) {
    Serial.print(F("Essa tecla ja foi mapeada como tecla ")); Serial.println(dup + 1);
    waitRelease();
    return;
  }

  keys[current].a = pa[0];
  keys[current].b = pb[0];
  keys[current].ab = m[pa[0]][pb[0]];
  keys[current].ba = m[pb[0]][pa[0]];
  keys[current].mapped = true;

  Serial.print(F("OK tecla ")); Serial.print(current + 1);
  Serial.print(F(": pino ")); Serial.print(pa[0] + 1);
  Serial.print(F(" <-> pino ")); Serial.println(pb[0] + 1);

  current++;
  waitRelease();

  if (current >= TOTAL_KEYS) { printMap(); }
  else printPrompt();
}