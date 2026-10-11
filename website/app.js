let port, writer, reader, keepReading = false;

const log = document.getElementById('log');
const btnConectar = document.getElementById('conectar');
const btnDesconectar = document.getElementById('desconectar');
const btnEnviar = document.getElementById('enviar');
const inputTexto = document.getElementById('texto');

function escrever(msg) {
  log.textContent += msg;
  log.scrollTop = log.scrollHeight;
}

btnConectar.onclick = async () => {
  if (!('serial' in navigator)) {
    alert('Este navegador não suporta Web Serial. Use Chrome ou Edge.');
    return;
  }
  try {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: 9600 }); // mesmo valor do Serial.begin()

    // Escrita
    const encoder = new TextEncoderStream();
    encoder.readable.pipeTo(port.writable);
    writer = encoder.writable.getWriter();

    btnConectar.disabled = true;
    btnDesconectar.disabled = false;
    btnEnviar.disabled = false;
    inputTexto.disabled = false;

    lerDados();
  } catch (e) {
    escrever('\n[Erro] ' + e.message + '\n');
  }
};

async function lerDados() {
  const decoder = new TextDecoderStream();
  port.readable.pipeTo(decoder.writable);
  reader = decoder.readable.getReader();
  keepReading = true;

  try {
    while (keepReading) {
      const { value, done } = await reader.read();
      if (done) break;
      if (value) escrever(value);
    }
  } catch (e) {
    escrever('\n[Erro de leitura] ' + e.message + '\n');
  } finally {
    reader.releaseLock();
  }
}

btnEnviar.onclick = async () => {
  const msg = inputTexto.value;
  if (!msg) return;
  await writer.write(msg + '\n');
  inputTexto.value = '';
};

inputTexto.addEventListener('keydown', e => {
  if (e.key === 'Enter') btnEnviar.click();
});

btnDesconectar.onclick = async () => {
  keepReading = false;
  await reader.cancel();
  await writer.close();
  await port.close();

  btnConectar.disabled = false;
  btnDesconectar.disabled = true;
  btnEnviar.disabled = true;
  inputTexto.disabled = true;
};