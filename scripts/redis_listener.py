#!/usr/bin/python

import redis
import time

r = redis.StrictRedis(host='localhost',port=6379)
p = r.pubsub()
p.psubscribe( '*' )

while True:
	for item in p.listen():
		print item['channel'], ":", item['data']
	time.sleep(0.001)

