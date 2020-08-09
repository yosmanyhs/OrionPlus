#include "BlockQueue.h"
#include "Block.h"

/* Constructors */
BlockQueue::BlockQueue()
{
    head_i = tail_i = length = 0;
    isr_tail_i = tail_i;
    ring = NULL;
}

BlockQueue::BlockQueue(unsigned int length)
{
    head_i = tail_i = 0;
    isr_tail_i = tail_i;

    ring = new Block[length];
    // TODO: handle allocation failure
    this->length = length;
}

/* Destructor */
BlockQueue::~BlockQueue()
{
    head_i = tail_i = length = 0;
    isr_tail_i = tail_i;

    if(ring != NULL)
        delete [] ring; // delete [] ring;

    ring = NULL;
}

/*
 * index accessors (protected)
 */

unsigned int BlockQueue::next(unsigned int item) const
{
    if (length == 0)
        return 0;

    if (++item >= length)
        return 0;

    return item;
}

unsigned int BlockQueue::prev(unsigned int item) const
{
    if (length == 0)
        return 0;

    if (item == 0)
        return (length - 1);
    else
        return (item - 1);
}

/*
 * reference accessors
 */
Block& BlockQueue::head()
{
    return ring[head_i];
}

Block& BlockQueue::tail()
{
    return ring[tail_i];
}

Block& BlockQueue::item(unsigned int i)
{
    return ring[i];
}

/*
 * pointer accessors
 */
Block* BlockQueue::head_ref()
{
    return &ring[head_i];
}

Block* BlockQueue::tail_ref()
{
    return &ring[tail_i];
}

Block* BlockQueue::item_ref(unsigned int i)
{
    return &ring[i];
}


void BlockQueue::produce_head(void)
{
    while (is_full())
        ;

    head_i = next(head_i);
}

void BlockQueue::consume_tail(void)
{
    if (!is_empty())
        tail_i = next(tail_i);
}


/*
 * queue status accessors
 */
bool BlockQueue::is_empty(void) const
{
    //__disable_irq();
    bool r = (head_i == tail_i);
    //__enable_irq();

    return r;
}

bool BlockQueue::is_full(void) const
{
    //__disable_irq();
    bool r = (next(head_i) == tail_i);
    //__enable_irq();

    return r;
}

void BlockQueue::flush_now()
{
    tail_i = head_i;
}

    /*
 * resize
 */
bool BlockQueue::resize(unsigned int new_size)
{
    if (is_empty())
    {
        if (new_size == 0)
        {
            //__disable_irq();

            if (is_empty()) // check again in case something was pushed
            {
                head_i = tail_i = this->length = 0;

                //__enable_irq();

                if (ring != NULL)
                    delete [] ring;

                ring = NULL;

                return true;
            }

            //__enable_irq();

            return false;
        }

        // Note: we don't use realloc so we can fall back to the existing ring if allocation fails
        Block* newring = new Block[new_size];

        if (newring != NULL)
        {
            Block* oldring = ring;

            //__disable_irq();

            if (is_empty()) // check again in case something was pushed while malloc did its thing
            {
                ring = newring;
                this->length = new_size;
                head_i = tail_i = 0;

                //__enable_irq();

                if (oldring != NULL)
                    delete [] oldring;

                return true;
            }

            //__enable_irq();

            delete [] newring;
        }
    }

    return false;
}

