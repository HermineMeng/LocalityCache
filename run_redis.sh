kill -9 $(pidof redis-server)

# proxy 1
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10010
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10011
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10012
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10013
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10014
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10015
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10016
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10017
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10018
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10019
 
# proxy 2
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10110
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10111
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10112
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10113
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10114
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10115
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10116
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10117
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10118
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10119

# proxy 3
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10210
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10211
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10212
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10213
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10214
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10215
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10216
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10217
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10218
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10219

# proxy 4
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10310
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10311
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10312
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10313
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10314
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10315
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10316
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10317
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10318
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10319

##################################################################################
# proxy 1
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10000
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10001
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10002
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10003
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10004
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10005
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10006
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10007
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10008
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10009
 
# proxy 2
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10100
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10101
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10102
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10103
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10104
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10105
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10106
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10107
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10108
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10109

# proxy 3
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10200
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10201
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10202
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10203
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10204
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10205
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10206
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10207
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10208
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10209

# proxy 4
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10300
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10301
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10302
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10303
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10304
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10305
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10306
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10307
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10308
./3rd_party/redis/bin/redis-server --daemonize yes --bind 0.0.0.0 --port 10309