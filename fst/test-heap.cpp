// range heap example
#include <iostream>     // std::cout
#include <algorithm>    // std::make_heap, std::pop_heap, std::push_heap, std::sort_heap
#include <vector>       // std::vector

class Compare
{
public:
	bool operator()(const int x, const int y) const
	{
		std::cout << x << " " << y << std::endl;
		return x > y;
	}
};
int main () 
{
	Compare compare;
	int myints[] = {10,20,30,5,15};
	std::vector<int> v(myints,myints+5);

	std::make_heap (v.begin(),v.end(),compare);
	std::cout << "initial max heap   : " << v.front() << '\n';

	for (unsigned i=0; i<v.size(); i++)
		std::cout << ' ' << v[i];
	std::cout << '\n';
	std::pop_heap (v.begin(),v.end(),compare); v.pop_back();
	std::cout << "max heap after pop : " << v.front() << '\n';
	for (unsigned i=0; i<v.size(); i++)
		std::cout << ' ' << v[i];
	std::cout << '\n';

	v.push_back(99); std::push_heap (v.begin(),v.end(),compare);
	std::cout << "max heap after push: " << v.front() << '\n';
	for (unsigned i=0; i<v.size(); i++)
		std::cout << ' ' << v[i];
	std::cout << '\n';

	std::sort_heap (v.begin(),v.end(),compare);

	std::cout << "final sorted range :";
	for (unsigned i=0; i<v.size(); i++)
		std::cout << ' ' << v[i];
	std::cout << '\n';

	return 0;
}
