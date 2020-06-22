#ifndef UI_TASK_H
#define UI_TASK_H

class TestClass
{
    public:
        TestClass(const char * info);
        ~TestClass();
    
        void doSomething(int k);
        int getResult();
    
    protected:
        char* m_data;
        int m_value;
};

void UI_BootTask_Entry(void * pvParam);

#endif
