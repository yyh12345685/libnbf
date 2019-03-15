#!/usr/bin/python

import http.client
import time

class HttpClient:
	host = '127.0.0.1'
	port = 9090
	
	def Process(self):
		urls = ['/as?adsid=m_FC_mm&impid=601f607dab334f53899f24808124a7c2&dspid=102&crid=21234568&adverid=101&uid=0A1vI4IUaT8F0c%3D%3D&p=bc&url=&sig=6d5c1e6a104356967eedfa822ae4db23',
				'/as?adsid=m_FC_mm&impid=601f607dab334f53899f24808124a7c2&dspid=102&crid=21234568&adverid=101&uid=0A1vI4IUaT8F0c%3D%3D&p=bc&url=&sig=6d5c1e6a104356967eedfa822ae4db23',
				'/as?adsid=m_FC_mm&impid=601f607dab334f53899f24808124a7c2&dspid=102&crid=21234568&adverid=101&uid=0A1vI4IUaT8F0c%3D%3D&p=bc&url=&sig=6d5c1e6a104356967eedfa822ae4db23',
				'/ac?adsid=m_FC_mm&impid=601f607dab334f53899f24808124a7c2&dspid=102&crid=21234568&adverid=101&uid=0A1vI4IUaT8F0c%3D%3D&p=bc&url=&sig=6d5c1e6a104356967eedfa822ae4db23&click=http%3a%2f%2fwww.dsp2.com%2fc%3faa%3dxx%26bb%3dyy%26u%3dwww.baidu.com/abc.html',
				'/ac?adsid=m_FC_mm&impid=601f607dab334f53899f24808124a7c2&dspid=102&crid=21234568&adverid=101&uid=0A1vI4IUaT8F0c%3D%3D&p=bc&url=&sig=6d5c1e6a104356967eedfa822ae4db23&click=http%3a%2f%2fwww.dsp2.com%2fc%3faa%3dxx%26bb%3dyy%26u%3dwww.baidu.com/abc.html']
		idx = 0
		section_times = 0
		qps_time = time.time()
		headers={
				"Host":"127.0.0.1:9090",
				"User-Agent":"Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:38.0) Gecko/20100101 Firefox/38.0",
				"Connection":"keep-alive",
				"Accept-Language":"en-US,en;q=0.5",
				"Accept":"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
				}

		print ("connect server:%s,port:%s" % (self.host,self.port))

		while(True):
			http_cli = http.client.HTTPConnection(self.host,self.port,timeout=10)
			http_cli.request('GET',urls[idx%5],'',headers)
			idx += 1
			section_times += 1
			response = http_cli.getresponse()
			print ("response info %s:" % idx)
			print (response.status)
			print (response.reason)
			#response.read()
			print (response.read())
			print ('\n')
			#time.sleep(5)
			if section_times > 10000:
				cur_time = time.time()
				qps = (cur_time-qps_time)/section_times
				section_times = 0
				qps_time = cur_time
				print ("qps %s" % qps)

			if idx >= 1000000:
				http_cli.close()
				break
			http_cli.close()
		print ("send times:%s" % idx)
		pass

def main():
	client = HttpClient()
	client.Process()

if __name__ == '__main__':
	main()
