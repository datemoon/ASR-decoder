#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "lat/kaldi-lattice.h"
#include "lat/push-lattice.h"
#include "fstext/rand-fst.h"

#include "src/post-processing-service/post-package.h"

using namespace kaldi;
using namespace fst;
#ifdef NAMESPACE
using namespace datemoon;
#endif
uint binary;

Lattice *RandLattice()
{
	RandFstOptions opts;
	opts.n_states = 10;
	opts.n_arcs = 30;
	opts.n_syms = 5;
	opts.acyclic = true;
	opts.allow_empty= false;
	Lattice *fst = fst::RandPairFst<LatticeArc>(opts);
	return fst;
}

CompactLattice *RandCompactLattice() 
{
	Lattice *fst = RandLattice();
	CompactLattice *cfst = new CompactLattice;
	ConvertLattice(*fst, cfst);
	delete fst;
	return cfst;
}

bool AsrReturnPackageAnalysisTest(
		bool set_nbest=true, bool set_lattice=true, bool compact_lattice = true, std::string prefix="")
{
	std::cout << "---------------------------------------------------------" << std::endl; 
	std::cout << prefix << " test:" << std::endl; 
	// 构造lattice
	CompactLattice *clat = RandCompactLattice();
	Lattice *lat = RandLattice();
	if(compact_lattice == true)
	{
		fst::FstPrinter<CompactLatticeArc> printer(*clat, NULL, NULL,
				NULL, true, true, "\t");
		printer.Print(&std::cout, "<unknown>");
	}
	else
	{
		fst::FstPrinter<LatticeArc> printer(*lat, NULL, NULL,
				NULL, true, true, "\t");
		printer.Print(&std::cout, "<unknown>");
	}
	// 构造nbest
	std::vector<std::string> nbest_str({"hello","hi","hello every body"});

	AsrReturnPackageAnalysis cli, ser;
	const char *tmpfile = "asrtmp.io";
	int fd = open(tmpfile, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXG|S_IRWXU|S_IRWXU);
	if(fd < 0)
	{
		std::cout << "open " << tmpfile << " failed." << std::endl;
		return false;
	}

	if(set_nbest == true)
	{
		ser.SetNbest(nbest_str);
	}

	if(set_lattice == true)
	{
		if(compact_lattice == true)
			ser.SetLattice(clat,binary);
		else
			ser.SetLattice(lat, binary);
	}

	ser.Print(prefix+"ser->");
	// write to fd
	if(ser.Write(fd, S2CEND) < 0)
	{
		std::cerr << "ser.Write failed!!!" << std::endl;
		return false;
	}
	ser.Reset();
	delete clat;
	delete lat;
	close(fd);

	fd = open(tmpfile, O_RDONLY);
	if(fd < 0)
	{
		std::cout << "open " << tmpfile << " failed." << std::endl;
		return false;
	}

	// read from fd
	if(cli.Read(fd) < 0)
	{
		std::cerr << "cli.Read failed!!!" << std::endl;
		return false;
	}
	cli.Print(prefix+"cli->");

	std::vector<std::string> cli_nbest_res;
	CompactLattice *cli_clat=NULL;
	Lattice *cli_lat=NULL;
	cli.GetData(&cli_nbest_res, cli_clat, cli_lat);
	if(compact_lattice == true)
	{
		if(cli_clat != NULL)
		{
			fst::FstPrinter<CompactLatticeArc> printer(*cli_clat, NULL, NULL,
					NULL, true, true, "\t");
			printer.Print(&std::cerr, "<unknown>");
		}
	}
	else
	{
		if(cli_lat != NULL)
		{
			fst::FstPrinter<LatticeArc> printer(*cli_lat, NULL, NULL,
					NULL, true, true, "\t");
			printer.Print(&std::cerr, "<unknown>");
		}
	}
	cli.Reset();
	close(fd);
	return true;
}

int main(int argc, char * argv[])
{
	int n = 100;
	binary = 1;
	for(int i = 0; i < n; ++i)
	{
		if(AsrReturnPackageAnalysisTest(true, true, true, "0:true-true-true->") != true)
		{
			std::cerr << "AsrReturnPackageAnalysisTest failed!!!" << std::endl;
			return -1;
		}
		if(AsrReturnPackageAnalysisTest(true, true, false, "1:true-true-false->") != true)
		{
			std::cerr << "AsrReturnPackageAnalysisTest failed!!!" << std::endl;
			return -1;
		}
		if(AsrReturnPackageAnalysisTest(true, false, true, "2:true-false->") != true)
		{
			std::cerr << "AsrReturnPackageAnalysisTest failed!!!" << std::endl;
			return -1;
		}
		if(AsrReturnPackageAnalysisTest(false, true, true, "3:false-true-true->") != true)
		{
			std::cerr << "AsrReturnPackageAnalysisTest failed!!!" << std::endl;
			return -1;
		}
		if(AsrReturnPackageAnalysisTest(false, true, false, "4:false-true-false->") != true)
		{
			std::cerr << "AsrReturnPackageAnalysisTest failed!!!" << std::endl;
			return -1;
		}
	}
	return 0;
}
