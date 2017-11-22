#ifndef   __aaFEATURE_EXTACTOR_Haa__
#define  __aaFEATURE_EXTACTOR_Haa__


namespace tasr{
enum EnumFeatError
{
	FEATEXACTOR_SUCC = 0,	         //提特征成功
	FEATEXACTOR_BUFF_OVERFLOW = -1,  //音频数据或特征数据存储buff溢出
	FEATEXACTOR_FAIL = -2	         //提取特征失败，理论上不该有这种错误，除非传入的参数不合法
};

class FrontEnd;
class FeatureExtractor
{
public:
	/*
	初始化函数，返回0表示成功，返回-1表示失败
	Input:    配置文件路径
	Output:   
	*/

	FeatureExtractor();
	~FeatureExtractor();
	int Init(const char*cfgPath,const char *sysDir);

	
	/*
	逆初始化函数
	*/
	void Uninit();
    
	/*
	重置操作，每句话开始必须调用， 准备为下一句语音提特征.返回0表示成功，返回-1表示失败
	*/
	int Reset();

	/*
	特征数据的存储buff和对应大小[以字节为单位]，空间由外边开辟和释放，大小也由外边决定。
	返回0表示成功，返回-1表示失败.
	buff的作用是：当有新数据来临时，提取出的特征数据放入featbuff。
	*/
	int SetBuff(void* featbuff,int featbuff_size);

	/*
	获取特征的维数，根据配置文件的特征产生配置得到
	*/
	int GetFeatureDim();


	/*TASRParamKind */
	int GetTgtKind();

	/*
	  获取音频的采样率, 采样率实际上从配置文件设置，一般8000或者16000
	*/
	int GetSrcSampleRate();

	/*
	  获取帧的速率，一般值，100
	*/
	int GetTgtFrmRate();

	/*
	特征提取函数，wavdata是新送进来的音频数据，wav_len是其长度，以字节为单位,
	新提取出的特征追加到SetBuf时设置的featbuff里，featFrmNum是新提取出来的特征长度,以帧为单位。
	在提特征操作之前，需要检查下featbuff是否会溢出，如果溢出则不进行提特征操作，返回FEATEXACTOR_BUFF_OVERFLOW。
	否则正常提取，成功返回FEATEXACTOR_SUCC，失败返回FEATEXACTOR_FAIL。
	如果传进来的数据不够提取1帧特征，则也返回FEATEXACTOR_SUCC，同时feature_len = 0。
	wav_len可以为0，wavdata可以为NULL，返回FEATEXACTOR_SUCC，同时feature_len = 0。
	*/
	int ExtractFeat(char* wavdata,int wav_len,int& featFrmNum);

	/*
	该函数同ExtractFeat，唯一区别是ExtractFeat_Last是收到语音结束信号后的提取。
	*/
	int ExtractFeat_Last(char* wavdata,int wav_len,int& featFrmNum);

private:
	//The external memory
	void * m_FeatBuf;
	int    m_FeatBufSize;


	//The actually object
	FrontEnd *m_pFrontEnd;
};
}
#endif