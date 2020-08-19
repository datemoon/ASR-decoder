#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/nnet/nnet-layer.h"
#include "src/nnet/lstm-layer.h"
#include "src/nnet/nnet-component.h"
#include "src/nnet/cudnn-lstm-layer.h"
#include "src/nnet/lstm-projected-layer.h"
#include "src/nnet/nnet-simple-recurrent.h"
#include "src/nnet/tf-lstm-layer.h"

#include "src/util/namespace-start.h"

Component* Component::NewComponentOfType(ComponentType comp_type, int input_dim, int output_dim)
{
	Component *ans = NULL;
	switch (comp_type) 
	{
		case kAffineTransform :
			ans = new AffineTransform(input_dim, output_dim);
			break;
		case kLinearTransform :
			ans = new LinearTransform(input_dim, output_dim);
			break;
		case kSoftmax :
			ans = new Softmax(input_dim, output_dim);
			break;
		case kSigmoid :
			ans = new Sigmoid(input_dim, output_dim);
			break;
		case kSplice :
			ans = new Splice(input_dim, output_dim);
			break;
		case kAddShift :
			ans = new AddShift(input_dim, output_dim);
			break;
		case kRescale :
			ans = new Rescale(input_dim, output_dim);
			break;
		case kPrior :
			ans = new Prior(input_dim, output_dim);
			break;
		case kLstm :
			ans = new Lstm(input_dim, output_dim);
			break;
		case kLstmCudnn :
			ans = new LstmCudnn(input_dim, output_dim);
			break;
		case kLstmProjectedStreams:
			ans = new LstmProjectedStreams(input_dim, output_dim);
			break;
		case kSRU:
			ans = new SRUcell(input_dim, output_dim);
			break;
		case kTfLstm:
			ans = new TfLstm(input_dim, output_dim);
			break;
		case kUnknown :
		default:
			break;
	}
	return ans;
}

Component* Component::Read(FILE *fp)
{
	int dim_in,dim_out;

	int token;
	if(feof(fp))
		return NULL;
	if(1 != fread(&dim_in,sizeof(int),1,fp))
		return NULL;
	if(1 != fread(&dim_out,sizeof(int),1,fp))
		return NULL;
	if(1 != fread(&token,sizeof(int),1,fp))
		return NULL;

	Component *ans = NewComponentOfType((ComponentType)token, dim_in, dim_out);

	if(ans == NULL)
		return NULL;
	if(true !=ans->ReadData(fp))
		return NULL;

	return ans;
}

bool Component::Write(FILE *fp)
{
	int token = (int)GetType();
	if(1 != fwrite(&_input_dim,sizeof(int),1,fp))
		return false;
	if(1 != fwrite(&_output_dim,sizeof(int),1,fp))
		return false;
	if(1 != fwrite(&token,sizeof(int),1,fp))
		return false;

	return WriteData(fp);
}

Component* Component::Read(FILE *fp, bool binary)
{
	const unsigned len = 1024;
	char str[len];
	memset(str,0x00, len * sizeof(char));
	ReadLine(fp, str, len);
	if(strncmp(str,"<Nnet>",6) == 0 || strncmp(str,"<!EndOfComponent>",17) == 0)
		ReadLine(fp, str, len);
	if(strncmp(str,"</Nnet>", 7) == 0)
		return NULL;
	int dim_in =0,dim_out=0;
	ComponentType type = GetTypeInOutDim(str,dim_in, dim_out,true);
	assert(type != kUnknown && "type is kUnknow");

	Component *ans = NewComponentOfType((ComponentType)type, dim_in, dim_out);
	if(ans == NULL)
		return NULL;
	if(true !=ans->ReadData(fp, binary))
		return NULL;

	return ans;

}

ComponentType GetTypeInOutDim(char *str,int &indim, int &outdim, bool essen)
{
	char *ptr = NULL;
	char *token = strtok_r(str," ",&ptr);
	ComponentType type;
	if(strncmp(token,"<Splice>", 8) == 0)
		type = kSplice;
	else if(strncmp(token,"<AddShift>", 10) == 0)
		type = kAddShift;
	else if(strncmp(token,"<Rescale>", 9) == 0)
		type = kRescale;
	else if(strncmp(token,"<Lstm>", 6) == 0)
		type = kLstm;
	else if(strncmp(token,"<Softmax>", 9) == 0)
		type = kSoftmax;
	else if(strncmp(token,"<Sigmoid>", 9) == 0)
		type = kSigmoid;
	else if(strncmp(token,"<AffineTransform>", 17) == 0)
		type = kAffineTransform;
	else if(strncmp(token,"<Prior>", 7) == 0)
		type = kPrior;
	else if(strncmp(token,"<LstmCudnn>", 11) == 0)
		type = kLstmCudnn;
	else if(strncmp(token,"<LstmProjectedStreams>", 22) == 0)
		type = kLstmProjectedStreams;
	else if(strncmp(token,"<LstmProjected>", 15) == 0)
		type = kLstmProjectedStreams;
	else if(strncmp(token,"<SimpleRecurrentUnit>",21) == 0)
		type = kSRU;
	else if(strncmp(token,"<TfLstm>", 8) == 0)
		type = kTfLstm;
	else
	{
		type = kUnknown;
		return type;
	}
	token = strtok_r(NULL," ", &ptr);
	if(strncmp(token,"<InputDim>", 10) == 0)
	{
		token = strtok_r(NULL," ", &ptr);
		indim = atoi(token);
	}
	else
	{
		outdim = atoi(token);
	}
	token = strtok_r(NULL," ", &ptr);
	if(strncmp(token,"<OutputDim>", 11) == 0 || strncmp(token,"<CellDim>", 9) == 0)
	{
		token = strtok_r(NULL," ", &ptr);
		outdim = atoi(token);
	}
	else
	{
		indim = atoi(token);
	}
	return type;
}

void Component::Propagate(float *in,int frames,int dim,float *out,int &outdim)
{
	// do nothing check
	assert(dim == _input_dim);
	outdim = _output_dim;
	PropagateFnc(in,frames,dim,out);
}
#include "src/util/namespace-end.h"
