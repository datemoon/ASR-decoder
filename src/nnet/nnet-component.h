#ifndef __NNET_COMPONENT_H__
#define __NNET_COMPONENT_H__

#include <string>
#include "src/util/namespace-start.h"

using namespace std;
typedef enum
{
	kUnknown = 0x00,

	kAffineTransform = 0x0100,
	kLinearTransform,
	kLstmProjectedStreams,
	kLstm,
	kLstmCudnn,
	kSRU,
	kTfLstm,
	kLstmProjected,

	kActivationFunction = 0x0200,
	kSoftmax,
	kSigmoid,

	kTranform = 0x0400,
	kSplice,
	kAddShift,
	kRescale,

	kPrior = 0x0600
}ComponentType;

class Component
{
public:
	Component(int input_dim, int output_dim):
		_input_dim(input_dim), _output_dim(output_dim) { }

	virtual ~Component() { }

	virtual ComponentType GetType() const = 0;

	virtual void GetType(string &type) const = 0;
	
	int InputDim() { return _input_dim; }

	int OutputDim() { return _output_dim; }

	void Propagate(float *in,int frames,int dim,float *out,int &outdim) ;

	static Component* Read(FILE *fp);
	
	static Component* Read(FILE *fp, bool binary);

	bool Write(FILE *fp);


	virtual int GetLeft() {return 0;}
	virtual int GetRight() {return 0;}
protected:
	virtual bool ReadData(FILE *fp) { return true;}

	virtual bool ReadData(FILE *fp, bool binary) { return true;}

	virtual bool WriteData(FILE *fp) { return true;}
private:
	virtual void PropagateFnc(float *in, int frames, int cols, float *out ) = 0;
private:
	int _input_dim;
	int _output_dim;
private:
	static Component* NewComponentOfType(ComponentType t, int input_dim, int output_dim);
};
ComponentType GetTypeInOutDim(char *str,int &indim, int &outdim, bool essen);
#include "src/util/namespace-end.h"
#endif
