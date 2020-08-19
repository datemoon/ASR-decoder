#include "src/hmm/hmm-topology.h"
#include "src/util/io-funcs.h"
#include <vector>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <utility>
#include <algorithm>
#include <assert.h>

#include "src/util/namespace-start.h"

void HmmTopology::Read(std::istream &is, bool binary)
{
	ExpectToken(is, binary, "<Topology>");
	if(!binary)
	{
		_phones.clear();
		_phone2idx.clear();
		_entries.clear();
		std::string token;
		while ( ! (is >> token).fail() )
		{
			if (token == "</Topology>") 
			{ 
				break; // finished parsing.
			}
			else  if (token != "<TopologyEntry>")
			{
				std::cerr << "Reading HmmTopology object, expected </Topology> or <TopologyEntry>, got "<<token << std::endl;
			}
			else
			{
				ExpectToken(is, binary, "<ForPhones>");
				std::vector<int> phones;
				std::string s;
				while(1)
				{
					is >> s;
					if (is.fail())
						std::cerr << "Reading HmmTopology object, unexpected end of file while expecting phones." << std::endl;
					if (s == "</ForPhones>") 
						break;
					else 
					{
						int phone = atoi(s.c_str());
						if(phone < 0)
						{
							std::cerr << "Reading HmmTopology object, expected "
								<< "integer, got instead " << s;
						}
						phones.push_back(phone);
					}
				}// while(1)
				std::vector<HmmState> this_entry;
				std::string token;
				ReadToken(is, binary, &token);
				while(token != "</TopologyEntry>")
				{
					if(token != "<State>")
						std::cerr << "Expected </TopologyEntry> or <State>, got instead "<<token << std::endl;

					int state;
					is >> state;
					if (state != static_cast<int>(this_entry.size()))
						std::cerr << "States are expected to be in order from zero, expected "
							<< this_entry.size() <<  ", got " << state << std::endl;
					ReadToken(is, binary, &token);
					int pdf_class = -1; // -1 by default, means no pdf.
					if (token == "<PdfClass>")
					{
						is >> pdf_class;
						ReadToken(is, binary, &token);
					}

					this_entry.push_back(HmmState(pdf_class));
					while (token == "<Transition>")
					{
						int dst_state;
						float trans_prob;
						is >> dst_state;
						is >> trans_prob;
						this_entry.back()._transitions.push_back(std::make_pair(dst_state, trans_prob));
						ReadToken(is, binary, &token);
					}

					if (token != "</State>")
					   std::cerr << "Reading HmmTopology,  unexpected token "<<token << std::endl;
					ReadToken(is, binary, &token);
				}
				int my_index = _entries.size();				
				_entries.push_back(this_entry);

				for (size_t i = 0; i < phones.size(); i++) 
				{
					int phone = phones[i];
					if (static_cast<int>(_phone2idx.size()) <= phone)
						_phone2idx.resize(phone+1, -1);  // -1 is invalid index.
					assert(phone > 0);
					if(_phone2idx[phone] != -1)
						std::cerr << "Phone with index "<<(i)<<" appears in multiple topology entries." << std::endl;

					_phone2idx[phone] = my_index;
					_phones.push_back(phone);
				}
			}
		}// while()
		std::sort(_phones.begin(), _phones.end());
	}// if (!binary)
}

void HmmTopology::Write(std::ostream &os, bool binary) const
{
	WriteToken(os, binary, "<Topology>");
	if (!binary) 
	{  // Text-mode write.
		os << "\n";
		for (int i = 0; i < static_cast<int> (_entries.size()); i++)
		{
			WriteToken(os, binary, "<TopologyEntry>");
			os << "\n";
			WriteToken(os, binary, "<ForPhones>");
			os << "\n";
			for (size_t j = 0; j < _phone2idx.size(); j++)
			{
				if (_phone2idx[j] == i)
					os << j << " ";
			}
			os << "\n";
			WriteToken(os, binary, "</ForPhones>");
			os << "\n";
			for (size_t j = 0; j < _entries[i].size(); j++)
			{
				WriteToken(os, binary, "<State>");
				WriteBasicType(os, binary, static_cast<int>(j));
				if (_entries[i][j]._pdf_class != -1) 
				{
					WriteToken(os, binary, "<PdfClass>");
					WriteBasicType(os, binary, _entries[i][j]._pdf_class);
				}
				for (size_t k = 0; k < _entries[i][j]._transitions.size(); k++) 
				{
					WriteToken(os, binary, "<Transition>");
					WriteBasicType(os, binary, _entries[i][j]._transitions[k].first);
					WriteBasicType(os, binary, _entries[i][j]._transitions[k].second);
				}
				WriteToken(os, binary, "</State>");
				os << "\n";
			}
			WriteToken(os, binary, "</TopologyEntry>");
			os << "\n";
		}
	}
	else
	{
		std::cerr << "now it's not support binary format." << std::endl;
	}
	WriteToken(os, binary, "</Topology>");
	if (!binary) 
		os << "\n";
}

const HmmTopology::TopologyEntry& HmmTopology::TopologyForPhone(int phone) const 
{  // Will throw if phone not covered.
	if (static_cast<size_t>(phone) >= _phone2idx.size() || _phone2idx[phone] == -1) {
		std::cerr << "TopologyForPhone(), phone "<<(phone)<<" not covered." << std::endl;
	}
	return _entries[_phone2idx[phone]];

}

#include "src/util/namespace-end.h"
