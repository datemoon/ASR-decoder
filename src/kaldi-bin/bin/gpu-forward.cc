#include <iostream>
#include <string>
//#include <torch/torch.h>
//#include <torch/script.h>

// kaldi
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "matrix/kaldi-matrix.h"

int main(int argc, char *argv[])
{
try
{
	const char *usage =
		"torch nnet forward calculate and use kaldi feature.\n"
		"Usage: gpu-forward [options] <feature-rspecifier> <nnetout-wspecifier>\n"
		"e.g.: gpu-forward ark:- ark,scp:foo.ark,foo.scp\n";

	kaldi::ParseOptions po(usage);
	bool binary = true;
	po.Read(argc, argv);

	if (po.NumArgs() != 2)
	{
		po.PrintUsage();
		exit(1);
	}

	kaldi::int32 num_done = 0;
	std::string rspecifier = po.GetArg(1);
	std::string wspecifier = po.GetArg(2);
	kaldi::SequentialBaseFloatMatrixReader kaldi_reader(rspecifier);
	for(; !kaldi_reader.Done(); kaldi_reader.Next(), num_done++)
	{
		kaldi::Matrix<kaldi::BaseFloat> mat = kaldi_reader.Value();
		std::string utt = kaldi_reader.Key();

	}
	return 0;
	/*
	if(argc != 2)
	{
		std::cerr << "usage : " << argv[0] << " torch-model" << std::endl;
		return -1;
	}
	std::string model_file(argv[1]);
	torch::jit::script::Module module;
	try
	{
		module = torch::jit::load(model_file);
	}
	catch(const c10::Error &e)
	{
		std::cerr << "error loading the model\n";
		return -1;
	}
	torch::Device device(torch::kCPU);
	torch::Device cpudevice(torch::kCPU);
	if (torch::cuda::is_available()) 
	{
		std::cout << "CUDA is available! Training on GPU." << std::endl;
		device = torch::Device(torch::kCUDA);
	}
	module.to(device);

	std::cout << "torch::jit::load " << model_file << " ok." <<std::endl;

	auto options = at::TensorOptions()
		.dtype(at::kFloat)
		.layout(at::kStrided)
		.device(at::kCPU)
		.requires_grad(false);

	std::vector<float> input_buf(1 * 10 * 280, 1.f);
	at::Tensor input_tensor = at::from_blob(input_buf.data(),{1,10,280}, options);

	std::vector<torch::jit::IValue> inputs;
	inputs.push_back(torch::ones({1,10,280}).to(device));
	//inputs.push_back(torch::ones({1,10,280}));
	
	at::Tensor output = module.forward(inputs).toTensor();
	std::cout << output << std::endl;
	output = output.to(cpudevice);
	inputs.clear();
	inputs.push_back(input_tensor.to(device));
	at::Tensor output1 = module.forward(inputs).toTensor();
	output1=output1.to(cpudevice);
	std::cout << "is contiguous " << output1.is_contiguous() << std::endl;
	std::cout << "shape is " << output1.sizes() << std::endl;
	std::cout << "strides is " << output1.strides() << std::endl;
	int64_t element_size_in_bytes = output1.element_size();
	at::IntArrayRef shape1 = output1.sizes();
	int nelem = output1.sizes().size();
	std::vector<int> shape(nelem);
	size_t mem_size = 1;
	for(size_t i = 0 ; i < nelem;++i)
	{
		shape[i] = static_cast<int>(shape1[i]);
		mem_size *= shape[i];
	}

	switch( output1.scalar_type())
	{
		case c10::kFloat:
		{
			float *data = new float[mem_size];
			memcpy(data, output1.data_ptr(), sizeof(float)*mem_size);
			for(size_t i=0;i<shape[0];i++)
			{
				for(size_t j=0;j<shape[1];j++)
				{
					for(size_t k=0;k<shape[2];k++)
					{
						std::cout << data[i*shape[1]*shape[2]+j*shape[2]+k] << " ";
					}
					std::cout << std::endl;
				}
				std::cout << std::endl;
			}
			delete[] data;
			break;
		}
		default:
		{
			std::cout << "type error!" << std::endl;
		}
	}
	//std::cout << output << std::endl;
	//std::cout << output1 << std::endl;
	return 0;
	*/
}
catch (const std::exception &e) 
{
	std::cerr << e.what();
	return -1;
}
}

