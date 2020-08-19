#ifndef __FST_QUEUE_H__
#define __FST_QUEUE_H__

#include <deque>
using std::deque;
#include <vector>
using std::vector;

// template <class S>
// class Queue {
//  public:
//   typedef typename S StateId;
//
//   // Ctr: may need args (e.g., Fst, comparator) for some queues
//   Queue(...);
//   // Returns the head of the queue
//   StateId Head() const;
//   // Inserts a state
//   void Enqueue(StateId s);
//   // Removes the head of the queue
//   void Dequeue();
//   // Updates ordering of state s when weight changes, if necessary
//   void Update(StateId s);
//   // Does the queue contain no elements?
//   bool Empty() const;
//   // Remove all states from queue
//   void Clear();
// };

#include "src/util/namespace-start.h"
enum QueueType 
{
	TRIVIAL_QUEUE = 0,         // Single state queue
	FIFO_QUEUE = 1,            // First-in, first-out queue
	LIFO_QUEUE = 2,            // Last-in, first-out queue
	SHORTEST_FIRST_QUEUE = 3,  // Shortest-first queue
	TOP_ORDER_QUEUE = 4,       // Topologically-ordered queue
	STATE_ORDER_QUEUE = 5,     // State-ID ordered queue
	SCC_QUEUE = 6,             // Component graph top-ordered meta-queue
	AUTO_QUEUE = 7,            // Auto-selected queue
	OTHER_QUEUE = 8
};

// QueueBase, templated on the StateId, is the base class shared by the
// queues considered by AutoQueue.
template <class S>
class QueueBase
{
public:
	typedef S StateId;

	QueueBase(QueueType type): _queue_type(type), _error(false) { }
	virtual ~QueueBase() { }
	StateId Head() const { return _Head();}
	void Enqueue(StateId s) {  _Enqueue(s);}
	void Dequeu() {  _Dequeue(); }
	void Updata(StateId s) { _Updata(s); }
	bool Empty() const { return _Empty(); }
	void Clear() { _Clear(); }
	QueueType Type() { return _queue_type; }
	bool Error() const { return _error; }
	void SetError(bool error) { _error = error;}

private:
	// This allows base-class virtual access to non-virtual derived-
	// class members of the same name. It makes the derived class more
	// efficient to use but unsafe to further derive.
	virtual StateId _head() const = 0;
	virtual void _Enqueue(StateId s) = 0;
	virtual void _Dequeue() = 0;
	virtual void _Updata(StateId s) = 0;
	virtual bool _Empty() const = 0;
	virtual void _Clear() = 0;

	QueueType _queue_type;
	bool _error;
};

// Automatic queue discipline, templated on the StateId. It selects a
// queue discipline for a given FST based on its properties.
template <class S>
class AutoQueue: public QueueBase<S>
{
public:
	typedef S StateId;

	// This constructor takes a state distance vector that, if non-null and if
	// the Weight type has the path property, will entertain the
	// shortest-first queue using the natural order w.r.t to the distance.
	template<class Arc, class ArcFilter>
	AutoQueue(const Fst<Arc> &fst, const vector<typename Arc::Weight> *distance,
			ArcFilter filter): QueueBase<S>(AUTO_QUEUE)
	{
		typedef typename Arc::Weight Weight;
		typedef S
	}
private:
	QueueBase<StateId> *_queue;
	vector<QueueBase<StateId>* > _queues;
	vector<StateId> _scc;

	template <class Arc, class ArcFilter, class Less>
	static void SccQueueType(const Fst<Arc> &fst, const vector<StateId> &scc,
			vecotr<QueueType> * queue_types, 
			ArcFilter filter, Less *less,
			bool *all_trivial, bool *unweighted);

	// This allows base-class virtual access to non-virtual derived-
	// class members of the same name. It makes the derived class more
	// efficient to use but unsafe to further derive.
	virtual StateId _Head() const { return Head();}

	virtual void _Enqueue(StateId s) {Enqueue(s);}
	
	virtual void _Dequeue() { Dequeue();}

	virtual void _Updata(StateId s) { Updata(s);}

	virtual bool _Empty() const { return Empty(); }

	virtual void _Clear() { return Clear(); }

//	DISALLOW_COPY_AND_ASSIGN(AutoQueue);
};
#include "src/util/namespace-end.h"
#endif
