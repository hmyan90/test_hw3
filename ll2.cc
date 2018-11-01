#include <cstddef>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <mutex>

#define ELEMS 100
#define ITER 10000000
#define RATIO 10000

std::mutex global_lock;

struct list {

    struct list_elem {
	struct list_elem *next_;
	int id_;

	list_elem() : id_(-1), next_(NULL) {};
	list_elem(int _id) : id_(_id), next_(NULL) {};
    };

    struct list_elem sentinel_;

    list () {};

    struct list_elem * lookup(int _id, struct list_elem **prev) {
	global_lock.lock();
	struct list_elem *cur = sentinel_.next_;
	*prev = &sentinel_;

	while (cur) {

	    if (cur->id_ == _id) {
			global_lock.unlock();
			return cur;
		}

	    *prev = cur;
	    cur = cur->next_;
	}
	global_lock.unlock();
	return cur;
    }

    struct list_elem * lookup(int _id) {
	struct list_elem *prev;

	return lookup(_id, &prev);
    }

    bool push_front(int _id) {
	if (lookup(_id))
	    return false;

	struct list_elem *elem = new list_elem(_id);
	global_lock.lock();
	elem->next_ = sentinel_.next_;
	sentinel_.next_ = elem;
	global_lock.unlock();
	return true;
    }

    bool remove(int _id) {
	struct list_elem *prev;
	struct list_elem *elem = lookup(_id, &prev);

	if (elem) {
		global_lock.lock();
	    prev->next_ = elem->next_;
	    elem->next_ = NULL;
		global_lock.unlock();
	    delete(elem);

	    return true;
	}

	return false;
    }

    void delete_elems() {
	struct list_elem *cur = sentinel_.next_;
	struct list_elem *prev;
	int deleted = 0;

	if (cur) { //skip sentinel
	    prev = cur;
	    cur = cur->next_;		
	}

	while (cur) {
	    delete(prev);
	    deleted++;
	    prev = cur;
	    cur = cur->next_;
	}
	if (prev) {
	    delete(prev);
	    deleted++;
	}
    }

};

void init (struct list *ll)
{
    unsigned int seed;

    for (int i = 0; i < ELEMS / 2; i++) {
	ll->push_front(rand_r(&seed));
    }
}

void test(struct list *ll, int tid, int num_threads)
{
    unsigned int seed = tid;

    for (int i = 0; i < ITER / num_threads; i++) {
	unsigned int task = rand_r(&seed) % RATIO;
	unsigned int id = rand_r(&seed) % ELEMS;

	switch (task) {
	case 0:
	    ll->push_front(id);
	    break;
	case 1:
	    ll->remove(id);
	    break;
	default:
	    ll->lookup(id);
	    break;
	}
    }
}

int main( int argc, const char* argv[] )
{
    for (int num_threads = 1; num_threads < 33; num_threads *= 2) {

        struct list *ll = new list();
        init(ll);

	    std::thread t[num_threads];

	    auto start = std::chrono::system_clock::now();

	    for (int i = 0; i < num_threads; i++) {
	        t[i] = std::thread(test, ll, i, num_threads);
		}

	    for (int i = 0; i < num_threads; i++) {
	        t[i].join();
	    }

        auto end = std::chrono::system_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << elapsed.count() << '\n';

        ll->delete_elems();
        delete(ll);
    }
}
