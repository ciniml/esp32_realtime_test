---
figureTitle:  "図"
tableTitle:   "表"
listingTitle: "リスト"
figPrefix:    "図"
tblPrefix:    "表"
lstPrefix:    "リスト"
...

# ESP32のタイマーを使いこなす

ESP32の開発環境 ESP-IDF 、および ESP-IDF の上に構築されている Arduino core for the ESP32 では、正確な時間間隔で処理を行うための仕組みがいくつか用意されています。
ただし、ESP32の無線通信処理に関連して、これらの機能を利用して正確な時間間隔で処理を行うためには、いくつか注意点があります。

## 環境

* ESP-IDF v3.3
* M5StickC (ESP32-PICO)

## 測定の準備：タイムスタンプの取得方法の確認

### タイムスタンプ取得関数 esp_timer_get_time 

ESP-IDFには、タイムスタンプを取得するAPIとして、 `esp_timer_get_time` が用意されています。esp_timer_get_time はESP32が起動してからの経過時間をマイクロ秒単位で返します。

正確な時間間隔で処理できているかを確認するためには、十分な正確さで時刻を計測する必要があります。
そこで、まずは esp_timer_get_time がどのように実装されているのかを確認します。

### esp_timer_get_time の実装

esp_timer_get_time は ESP-IDFの `components/esp32/esp_timer.c` で定義されています。内部では単に `esp_timer_impl_get_time` を呼び出しています。

```{#lst:esp_timer_get_time .c .numberLines caption="esp_timer_get_time"}
int64_t IRAM_ATTR esp_timer_get_time()
{
    return (int64_t) esp_timer_impl_get_time();
}
```

esp_timer_impl_get_time は ESP-IDFの `components/esp32/esp_timer_esp32.c` で定義されています。
内部では一定間隔でカウントしつづけるフリーランニング･カウンタのカウンタ値やフラグを読み取っています。

このフリーランニング･カウンタは、ESP32のペリフェラル･バス (APB) のクロックである 80[MHz] でカウントしつづけているので、分解能は 1/80[us] です。

esp_timer_impl_get_time内で呼び出されている `timer_overflow_happeded` 関数は、フリーランニング･カウンタのレジスタをいくつか呼んだ上で条件を満たすかどうかを返しています。

ESP32で使われているCPUコアである Xtensa LX6 の詳細な資料は入手出来ないため、CPUの命令ごとのサイクル数は正確には分かりませんが、少なくとも1命令あたり1CPUサイクルかかると仮定します。
timer_overflow_happened および esp_timer_impl_get_time 関数はそれぞれ 40命令 と 29命令の機械語にコンパイルされることを確認しています。

また esp_timer_impl_get_time 内の処理は、一度カウンタ値を読み出した後再度読み出したときに値が異なる場合に、最大2回実行されます。
ループが2回回った場合、タイムスタンプの元となるカウンタの値を取得するのは、ループ1回分の処理が終わった後となりますので、その分タイムスタンプの時間がずれます。
ループ1回分の命令は多く見積もっても timer_overflow_happened と esp_timer_impl_get_time 関数の命令数を越えないため、 多くとも 40+29 命令です。

実際には1サイクルで実行できる命令ばかりではありませんが、仮に平均2サイクルかかるとしたとしても、 (40+29) * 2 = 138サイクルとなります。
160[MHz]で実行している場合、138/160 [us] = 0.8625[us] ですので、タイムスタンプの時刻はばらついたとしても 1[us]弱のずれとなります。

以上より、160[MHz]動作の場合、esp_timer_get_time 関数で取得できるタイムスタンプは、1[us]の精度があると考えて良いことがわかりました。

```{#lst:esp_timer_impl_get_time .c .numberLines caption="esp_timer_impl_get_time"}
uint64_t IRAM_ATTR esp_timer_impl_get_time()
{
    uint32_t timer_val;
    uint64_t time_base;
    uint32_t ticks_per_us;
    bool overflow;

    do {
        /* Read all values needed to calculate current time */
        timer_val = REG_READ(FRC_TIMER_COUNT_REG(1));
        time_base = s_time_base_us;
        overflow = timer_overflow_happened();
        ticks_per_us = s_timer_ticks_per_us;

        /* Read them again and compare */
        /* In this function, do not call timer_count_reload() when overflow is true.
         * Because there's remain count enough to allow FRC_TIMER_COUNT_REG grow
         */
        if (REG_READ(FRC_TIMER_COUNT_REG(1)) > timer_val &&
                time_base == *((volatile uint64_t*) &s_time_base_us) &&
                ticks_per_us == *((volatile uint32_t*) &s_timer_ticks_per_us) &&
                overflow == timer_overflow_happened()) {
            break;
        }

        /* If any value has changed (other than the counter increasing), read again */
    } while(true);

    uint64_t result = time_base
                        + timer_val / ticks_per_us;
    return result;
}
// Check if timer overflow has happened (but was not handled by ISR yet)
static inline bool IRAM_ATTR timer_overflow_happened()
{
    if (s_overflow_happened) {
        return true;
    }

    return ((REG_READ(FRC_TIMER_CTRL_REG(1)) & FRC_TIMER_INT_STATUS) != 0 &&
              ((REG_READ(FRC_TIMER_ALARM_REG(1)) == ALARM_OVERFLOW_VAL && TIMER_IS_AFTER_OVERFLOW(REG_READ(FRC_TIMER_COUNT_REG(1))) && !s_mask_overflow) || 
               (!TIMER_IS_AFTER_OVERFLOW(REG_READ(FRC_TIMER_ALARM_REG(1))) && TIMER_IS_AFTER_OVERFLOW(REG_READ(FRC_TIMER_COUNT_REG(1))))));
}
```

## 一定周期で処理を実行する方法

ESP-IDF および Arduino core for the ESP32 上で 一定周期で処理を実行する方法として、以下の3つ方法があります。

1. vTaskDelayやdelayで次の周期の開始時刻まで待つ
2. FreeRTOSのソフトウェア･タイマ機能を使う
3. ESP-IDFの 高分解能タイマ機能を使う
4. ESP32のハードウェア・タイマの割り込みを使う

それぞれの方法について、次項以降で説明します。

### 方法1: vTaskDelayで次の周期の開始時刻まで待つ

一番単純な方法は、FreeRTOSのAPIである `vTaskDelay` 関数を使って、次の周期の開始時刻まで待つことです。

ただし、この方法では FreeRTOSのTickの分解能より細かい周期で処理を行うことができません。

ESP-IDFのデフォルトでは、FreeRTOSのTickの周期は10[ms]に設定されています。よって、10[ms]よりも低い分解能で十分な場合はこの方法を使うことができます。

### 方法2: FreeRTOSのソフトウェア･タイマ機能を使う

FreeRTOSは指定した時刻や一定周期で処理を実行するソフトウェア・タイマ機能があります。
機能としては、方法1と同様の処理をFreeRTOS側で自動的に行ってくれることに相当する機能となります。

よって、方法1と同様に、FreeRTOSのTickより細かい周期での処理を行うことはできません。

### 方法3: ESP-IDFの 高分解能タイマ機能を使う

ESP-IDFにはFreeRTOSのTickよりも細かい周期で処理を行うための機能として、高分解能タイマ (High Resolution Timer) が用意されています。

高分解能タイマの仕様やAPIについては、ESP-IDFのドキュメントに記載されています。
https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/system/esp_timer.html

高分解能タイマ機能は `esp_timer_` で始まるAPIとして提供されています。仕様としては最小50[us]間隔で処理を実行することが出来ます。
ただし、50[us]の周期で処理を実行する場合、高分解能タイマを管理するための処理によるオーバーヘッドで多くの処理時間を使うため、あまり実用的ではありません。
100[us]周期程度が限界と考えておくのがよいでしょう。

### 方法4: ESP32のハードウェア・タイマ割り込みを使う

ESP32にはいくつかのハードウェア･タイマがあります。そのうち、ESP-IDF上でアプリケーションが自由に使えるハードウェア･タイマが合計4つあります。
ハードウェア･タイマは2つずつ、計2グループに分けられます。

ハードウェア・タイマの機能は `timer_` で始まるAPiで制御します。
詳細はESP-IDFのドキュメントに記載されています。 https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/peripherals/timer.html?highlight=Timer

ハードウェア･タイマは高分解能タイマと異なり、直接割り込み処理を扱う必要がありますので、高分解能タイマよりも扱いにくい半面、細かな設定を行えます。

## 実験その1: 高分解能タイマの性能

### 実験プログラム概要

ESP-IDFの高分解能タイマの処理の周期の正確さがどれくらいなのか調べるため、ESP-IDF付属の無線LAN接続サンプル・コードを変更した実験用プログラムを作成しました。
元にしたサンプル・コードは ESP-IDFの `examples/wifi/getting_started/station` 以下にあるプログラムです。

サンプル・コードに対して追加した機能は次の通りです。

1. 高分解能タイマを初期化し、500[us]間隔で処理を実行するように設定します。
2. 無線通信を有効にして測定する場合は、無線LANを初期化し、アクセスポイントに接続します。
3. 周期処理ルーチンから後述の通知があるまで待機します。
4. タイマの周期処理の中で、`esp_timer_get_time` 関数を使ってタイムスタンプを取得し、前回周期処理を実行してからの経過時間を計算します。計算した経過時間を配列に保存しておきます。
5. 16384回、周期処理を終えたら、(3))で通知を待機しているメインタスクに通知します。
6. (4)で計測した結果をシリアル通信経由でターミナルに出力します。

このプログラムを `a. 無線LAN通信なし`, `b. 無線LAN通信あり(アクセスポイントに接続のみ)` および `c. 無線LAN通信あり(PCから1[MB/s]くらいのレートでUDPパケット受信)` の2つの設定で実行して、(3)で測定した周期処理の間隔がどのように変化するかを測定しました。

### 測定結果

以下に測定結果を示します。

### 結果：高分解能タイマは使い物にならない

測定結果より、無線通信無効時はそれなりの正確さで周期処理を実行できています。
一方、無線通信有効時は周期処理の間隔がかなり大きくずれています。これでは高分解能タイマを使う意味がありません。

なぜこのような結果になるのか、ESP-IDFの高分解能タイマの実装から読み解いてみます。

### 高分解能タイマの実装



# 参考文献

* Espressif Inc, ESP32 Technical Reference Manual V2.3(http://espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
* Espressif Inc, ESP-IDF Programming Guide v3.1 (https://docs.espressif.com/projects/esp-idf/en/v3.1/)
* Bluetooth SIG, Specification of the Bluetooth System (https://www.bluetooth.org/DocMan/handlers/DownloadDoc.ashx?doc_id=441541&_ga=2.2788979.2127376998.1526238267-1586719393.1526238267)
* 足立 英治	IoT時代の最新Bluetooth5	インターフェース2017年11月号 pp.85-92
* 井田 健太	IoT実験に便利！500円Wi-Fiに新タイプ登場	インターフェース2017年11月号 pp.20-30
* CC2650 SensorTag User's Guide (http://processors.wiki.ti.com/index.php/CC2650_SensorTag_User%27s_Guide)
* SensorTag attribute table (http://processors.wiki.ti.com/images/a/a8/BLE_SensorTag_GATT_Server.pdf)
* インターネット経由でSORACOM Harvestにデータが入れられるようになりました。 (https://blog.soracom.jp/blog/2018/07/04/inventory-update/)
* デバイスIDとデバイスシークレットを使用してHarvestにデータを送信する (https://dev.soracom.io/jp/start/inventory_harvest_with_keys/)