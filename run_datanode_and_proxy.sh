kill -9 $(pidof run_datanode)
kill -9 $(pidof run_proxy)

# datanodes in proxy 1
./build/run_datanode 0.0.0.0 9000
./build/run_datanode 0.0.0.0 9001
./build/run_datanode 0.0.0.0 9002
./build/run_datanode 0.0.0.0 9003
./build/run_datanode 0.0.0.0 9004
./build/run_datanode 0.0.0.0 9005
./build/run_datanode 0.0.0.0 9006
./build/run_datanode 0.0.0.0 9007
./build/run_datanode 0.0.0.0 9008
./build/run_datanode 0.0.0.0 9009

# datanodes in proxy 2
./build/run_datanode 0.0.0.0 9100
./build/run_datanode 0.0.0.0 9101
./build/run_datanode 0.0.0.0 9102
./build/run_datanode 0.0.0.0 9103
./build/run_datanode 0.0.0.0 9104
./build/run_datanode 0.0.0.0 9105
./build/run_datanode 0.0.0.0 9106
./build/run_datanode 0.0.0.0 9107
./build/run_datanode 0.0.0.0 9108
./build/run_datanode 0.0.0.0 9109

# datanodes in proxy 3
./build/run_datanode 0.0.0.0 9200
./build/run_datanode 0.0.0.0 9201
./build/run_datanode 0.0.0.0 9202
./build/run_datanode 0.0.0.0 9203
./build/run_datanode 0.0.0.0 9204
./build/run_datanode 0.0.0.0 9205
./build/run_datanode 0.0.0.0 9206
./build/run_datanode 0.0.0.0 9207
./build/run_datanode 0.0.0.0 9208
./build/run_datanode 0.0.0.0 9209

# datanodes in proxy 4
./build/run_datanode 0.0.0.0 9300
./build/run_datanode 0.0.0.0 9301
./build/run_datanode 0.0.0.0 9302
./build/run_datanode 0.0.0.0 9303
./build/run_datanode 0.0.0.0 9304
./build/run_datanode 0.0.0.0 9305
./build/run_datanode 0.0.0.0 9306
./build/run_datanode 0.0.0.0 9307
./build/run_datanode 0.0.0.0 9308
./build/run_datanode 0.0.0.0 9309

########################################
########################################
########################################
########################################
# cachenodes in proxy 1
./build/run_datanode 0.0.0.0 9010
./build/run_datanode 0.0.0.0 9011
./build/run_datanode 0.0.0.0 9012
./build/run_datanode 0.0.0.0 9013
./build/run_datanode 0.0.0.0 9014
./build/run_datanode 0.0.0.0 9015
./build/run_datanode 0.0.0.0 9016
./build/run_datanode 0.0.0.0 9017
./build/run_datanode 0.0.0.0 9018
./build/run_datanode 0.0.0.0 9019

# cachenodes in proxy 2
./build/run_datanode 0.0.0.0 9110
./build/run_datanode 0.0.0.0 9111
./build/run_datanode 0.0.0.0 9112
./build/run_datanode 0.0.0.0 9113
./build/run_datanode 0.0.0.0 9114
./build/run_datanode 0.0.0.0 9115
./build/run_datanode 0.0.0.0 9116
./build/run_datanode 0.0.0.0 9117
./build/run_datanode 0.0.0.0 9118
./build/run_datanode 0.0.0.0 9119

# cachenodes in proxy 3
./build/run_datanode 0.0.0.0 9210
./build/run_datanode 0.0.0.0 9211
./build/run_datanode 0.0.0.0 9212
./build/run_datanode 0.0.0.0 9213
./build/run_datanode 0.0.0.0 9214
./build/run_datanode 0.0.0.0 9215
./build/run_datanode 0.0.0.0 9216
./build/run_datanode 0.0.0.0 9217
./build/run_datanode 0.0.0.0 9218
./build/run_datanode 0.0.0.0 9219

# cachenodes in proxy 4
./build/run_datanode 0.0.0.0 9310
./build/run_datanode 0.0.0.0 9311
./build/run_datanode 0.0.0.0 9312
./build/run_datanode 0.0.0.0 9313
./build/run_datanode 0.0.0.0 9314
./build/run_datanode 0.0.0.0 9315
./build/run_datanode 0.0.0.0 9316
./build/run_datanode 0.0.0.0 9317
./build/run_datanode 0.0.0.0 9318
./build/run_datanode 0.0.0.0 9319

# run proxy
./build/run_proxy 0.0.0.0 50005
./build/run_proxy 0.0.0.0 50015
./build/run_proxy 0.0.0.0 50025
./build/run_proxy 0.0.0.0 50035