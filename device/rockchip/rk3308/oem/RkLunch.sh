wpa_supplicant -B -i wlan0 -c /data/cfg/wpa_supplicant.conf
#./wakeWordAgent -e gpio &
amixer cset name='DAC HPOUT Right Volume' 30
amixer cset name='DAC HPOUT Left Volume' 30
amixer cset name='ADC MIC Group 1 Left Volume' 3
amixer cset name='ADC MIC Group 1 Right Volume' 3
amixer cset name='ADC ALC Group 1 Left Volume' 20
amixer cset name='ADC ALC Group 1 Right Volume' 20
amixer cset name='ADC MIC Group 3 Left Volume' 3
amixer cset name='ADC MIC Group 3 Right Volume' 3
amixer cset name='ADC ALC Group 3 Left Volume' 20
amixer cset name='ADC ALC Group 3 Right Volume' 20
