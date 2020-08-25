#include <vector>
#include <iostream>

using namespace std;

int main()
{
	vector<int *> a;
	for(int i=0;i<10;++i)
	{
		int *b = new int(i);
		a.push_back(b);
		std::cout << *(a[i]) << std::endl;
	}
	{
		for(size_t i=0;i<a.size();++i)
			delete a[i];
		vector<int *> s;
		s.swap(a);

	}
	
	return 0;
}
