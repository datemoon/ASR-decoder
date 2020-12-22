
from ctypes import string_at
from ctypes import c_char_p
from ctypes import POINTER
from ctypes import c_short
from ctypes import c_int
from ctypes import byref
from ctypes import CDLL

import sys
import time


class AsrClient(object):
    def __init__(self, clib="/home/hu.bo/git-mm/asr-proj/src/build/install/lib/libclient.so", wavhead_len=0, package_size=16000*10, ip="127.0.0.1", port=8100):
        self.clib = clib
        self.wavhead_len = wavhead_len
        self.package_size = package_size
        self.ip = ip
        self.port = port
        self.sample_rate = 16000
        self.bit = 16
        self.libclient_so = CDLL(self.clib)

    def Connect(self):

        ret = self.libclient_so.TcpConnect(c_char_p(self.ip),c_int(self.port))
        if ret != 0:
            print("libclient_so.TcpConnect failed!!!")
            return False
        return True

    def Close(self):
        self.libclient_so.TcpClose()

    def ProcessData(self, data, eos = False):
        if data:
            c_data = POINTER(c_short)()
            c_data = c_char_p(data)
            c_data_len = len(data)/2

            self.libclient_so.SendPack(c_data, c_data_len)

        if eos == True:
            self.libclient_so.SendLastPack()
            ret_size = c_int(0)
            retBuf = self.libclient_so.GetResult(byref(ret_size))
            result = string_at(retBuf, ret_size.value)
            self.libclient_so.free(retBuf)
            return result
        return None

    def ProcessFile(self, infile):
        self.Connect()
        wav_sample = 0.0
        result = ""
        start_time = time.time()
        try:
            with open(infile, 'r') as fp:
                fp.read(self.wavhead_len) # loss wav head
                while True:
                    data = fp.read(self.package_size)
                    wav_sample += len(data)
                    if data:
                        result = self.ProcessData(data, eos=False)
                    else:
                        result = self.ProcessData(data, eos=True)
                        print("%s %s"%(infile , result))
                        break
        except IOError:
            print("Open %s failed!!!" % (infile))
            return None
        end_time = time.time()
        self.Close()
        process_times = end_time - start_time

        wav_time_s = wav_sample * 1.0 / (self.sample_rate * (self.bit/8))
        rt = process_times/wav_time_s
        print("wav time is %f, process time is %f,rt is :%f" % (wav_time_s, process_times, rt))
        return (process_times ,wav_time_s, rt, result)

if __name__ == '__main__':

    if len(sys.argv) != 3:
        print("%s %s %s" %(sys.argv[0], "wav.list", "output.txt"))
        sys.exit(1)
    
    wav_list = sys.argv[1]
    output = sys.argv[2]
    with open(output, 'w') as fp:
        for line in open(wav_list, 'r'):
            infile = line.strip()
            asr_client = AsrClient()
            ret = asr_client.ProcessFile(infile)
            if ret is None:
                continue
            else:
                process_times ,wav_time_s, rt, result = ret
                fp.write(infile + " " + str(wav_time_s) + " " + str(process_times) + " " + result + "\n")
#            time.sleep(5)







