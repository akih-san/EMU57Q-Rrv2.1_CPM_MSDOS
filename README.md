# EMU57Q-8088/V20<br>
<br>
PIC18F57Q43を使った8088/用のSBCを作成し、CPM-86とMD-DOS2.11を移植し公開しています。<br>
SBCをRev2.1にバージョンアップしました。<br>
8088/V20が4.9MHz、または8MHzで動作します。<br>

EMU57Q-8088/V20 Rev2.1ボード
![P 1](photo/P1020507.JPG)

Rev2.1では、新たにI2Cをサポートしています。<br>

SBCとI2Cモジュール
![P 2](photo/P1020510.JPG)

また、新規ファームウェアでは、起動時ＯＳを選択出来るようにしました。<br>
ファイルは、CPM-86用のディレクトリファイルと、MS-DOS用のディレクトリ<br>
ファイルが用意されています。<br>
両方のディレクトリファイルをSDカードにコピーすると、どちらのOSを<br>
起動時するのかをパワーオン／リセット時に聞いてきます。<br>
単一のディレクトリファイルをコピーした場合には、コピーしたOSが起動します。<br>
<br>

CP/M-86起動画面
![P 3](photo/selectCPM.png)

MS-DOS起動画面
![P 4](photo/selectMSDOS.png)

# I2Cのサポート<br>
MS-DOSのクロックデバイスをサポートするためには、リアルタイムクロック<br>
（RTC）が必要です。<br>
PIC18F57Q43にはRTC機能はありません。<br>
（タイマーを使って疑似的に1秒を作り出すことはできます。）<br>
商業用にI2C機能を使ったRTCモジュールが販売されています。<br>
PIC18F57Q43にはI2C機能があるので、これを使えばRTC機能を持たせることが可能となります。<br>
<br>
そういった理由から、I2Cをサポートすることにしました。<br>
また、I2Cを使えば、商業用に販売されている色々なデバイスを動かす<br>
ことが出来ます。<br>
（当然対応するプログラムを作成する必要がありますが・・・）<br>
RTCモジュールとしてサポートしているのは、DS1307を使用したモジュール<br>
となります。また、I2C経由でUSB-UARTに接続するモジュール（FT200-XD)の<br>
サポートも実装してあります。<br>
<br>
I2Cが使えるのは、MS-DOSのみとなっています。CPM-86では扱っていません。<br>
<br>
I2Cのサポートについては、@etoolsLab369さんのページが非常に参考になりました。<br>
感謝いたします。<br>
<br>
（18F27Q43 2台を使ってHost-Client通信）<br>
https://qiita.com/etoolsLab369/items/65befd8fe1cccd3afc33<br>
<br>
<br>
＜DS1307を使ったRTCモジュール＞
![P 5](photo/P1020512.JPG)
![P 6](photo/P1020513.JPG)

AE-DS1307：　秋月電子通商で入手できます。<br>
https://akizukidenshi.com/catalog/g/g115488/<br>
<br>
Tiny RTC modules：　amazon、その他有名webストアで入手できます。<br>
説明書も何もありませんが、amazonに図面入りのレビューがありました。<br>
<br>
<br>
＜FT200XDを使ったUSB-UART接続モジュール＞
![P 7](photo/P1020511.JPG)

MFT200XD：　マルツのwebストアで入手できます。amazonからでも入手できます。<br>
https://www.marutsu.co.jp/pc/i/243038/<br>
<br>
CJMCU-200：　amazon、その他有名webストアで入手できます。<br>
説明書も何もありませんでしたが、問題なく使えました。<br>
<br>
<br>
# ファームウェア
@hanyazouさんが作成したZ80で動作しているCP/M-80のFWをベースに、<br>
EMU8088_57Q用のFWとして動作するように修正を加えました。<br>
今回は、CPM-86とMS-DOSを、SDカード上にバイナリファイルとして扱っています。<br>
<br>
(CPM-86)<br>
--CPMDISKSディレクトリ<br>
　　CBIOS.BIN　　　　BIOSバイナリファイル<br>
　　CCP_BDOS.BIN　　　CCP, BDOSバイナリファイル<br>
　　UMON_CPM.BIN　　　UNIMONのバイナリファイル<br>
　　DRIVEA.DSK　　　　Aドライブのディスクイメージ<br>
　　DRIVEB.DSK　　　　Bドライブのディスクイメージ<br>
　　DRIVEI.DSK　　　　Iドライブのディスクイメージ<br>
　　DRIVEJ.DSK　　　　Jドライブのディスクイメージ<br>
<br>
(MS-DOS)<br>
--DOSDISKSディレクトリ<br>
　　IO.SYS　　　　　　IOデバイスドライバのバイナリファイル<br>
　　MSDOS.SYS　　　　MS-DOS本体のバイナリファイル<br>
　　UMON_DOS.BIN　　　UNIMONのバイナリファイル<br>
　　DRIVEA.DSK　　　　Aドライブのディスクイメージ<br>
　　DRIVEB.DSK　　　　Bドライブのディスクイメージ<br>
　　DRIVEC.DSK　　　　Cドライブのディスクイメージ<br>
　　DRIVED.DSK　　　　Dドライブのディスクイメージ<br>
<br>
MS-DOS、CPM-86については、以下を参照してください。<br>
<br>
https://github.com/akih-san/EMU8088_57Q_V2_CPM86<br>
https://github.com/akih-san/EMU8088_MSDOS211<br>
<br>
<br>
＜＠hanyazouさんのソース＞<br>
https://github.com/hanyazou/SuperMEZ80/tree/mez80ram-cpm<br>
<br>
＜@electrelicさんのユニバーサルモニタ(UNIMON)＞<br>
https://electrelic.com/electrelic/node/1317<br>
<br>
<br>
# 参考
・EMUZ80<br>
EUMZ80はZ80CPUとPIC18F47Q43のDIP40ピンIC2つで構成されるシンプルな<br>
コンピュータです。<br>
<br>
＜電脳伝説 - EMUZ80が完成＞  <br>
https://vintagechips.wordpress.com/2022/03/05/emuz80_reference  <br>
＜EMUZ80専用プリント基板 - オレンジピコショップ＞  <br>
https://store.shopping.yahoo.co.jp/orangepicoshop/pico-a-051.html<br>
<br>
・SuperMEZ80<br>
SuperMEZ80は、EMUZ80にSRAMを追加し、Z80をノーウェイトで動かすことができるメザニンボードです<br>
<br>
SuperMEZ80<br>
https://github.com/satoshiokue/SuperMEZ80<br>
<br>
