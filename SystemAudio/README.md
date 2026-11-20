SR850 SYSTEM REFERENCE — BY ZEZO HXCX
Windows 10/11 • Áudio de sistema (YouTube, Spotify, vídeos)

IMPORTANTE: Equalizer APO é necessário. Este aplicativo NÃO instala drivers
e NÃO intercepta o áudio por conta própria.

1. Instale Equalizer APO pelo site oficial https://sourceforge.net/projects/equalizerapo/
2. No Configurator do Equalizer APO, selecione SOMENTE a saída de reprodução
   M-Audio Fast Track MKII usada pelo Windows. Reinicie o computador se solicitado.
3. Confirme que o áudio de YouTube sai pela M-Audio usando o driver normal do Windows.
   ASIO pode contornar o Equalizer APO, portanto o processamento não é garantido
   em DAWs com driver ASIO.
4. Abra SR850-System-Reference.exe e clique ATIVAR CORREÇÃO SR850.
5. Para ouvir nos monitores sem equalização, clique DESATIVAR ou feche o programa.

Atenção: na Fast Track MKII, fones e monitores podem compartilhar o mesmo sinal.
Enquanto a correção estiver ligada, ambos podem ser equalizados.

O aplicativo acrescenta um bloco identificado ao config.txt do Equalizer APO e
remove APENAS esse bloco ao desativar/fechar. Um processo auxiliar tenta removê-lo
também se o aplicativo encerrar inesperadamente. Isso não oferece garantia absoluta
contra desligamento abrupto ou queda de energia. Na próxima inicialização do
aplicativo, ele remove blocos antigos.

A primeira execução pode precisar de permissão de administrador para escrever
em C:\Program Files\EqualizerAPO\config.

FIR: mesma geração tools/make_ir.py da versão estável do VST3 SR850.
Filtro: 48 kHz; Equalizer APO processa e adapta a saída do Windows.
Pré-ganho: -7 dB para margem dinâmica; a reprodução ficará mais baixa.
A curva é uma calibração por medições públicas e comparação tonal; não uma
medição individual do seu exemplar.

Não instale Equalizer APO no dispositivo de seus monitores caso sejam separados.
Se houver outros efeitos no config.txt, eles continuam ativos ao fechar o programa.
Para garantir áudio sem nenhum efeito, remova também os demais efeitos no APO.

Código-fonte: https://github.com/zezohc-rgb/SR850-NAM-Reference/tree/feature/sr850-system-audio
