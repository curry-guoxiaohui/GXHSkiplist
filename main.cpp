#include "skiplist.h"
#include <string>
using namespace std;

namespace gxh_test_insert
{
    template<typename K,typename V>
    void test_insert(SkipList<K,V>* list)
    {
        list->insert_element("22","aaaa");
        list->insert_element("22","aaaa");
        list->insert_element("33","bbbb");
        list->insert_element("33","eeeee");
        list->insert_element("55","eeeee");
        list->insert_element("66","eeeee");
        list->insert_element("77","eeeee");
    }
}

namespace gxh_test_delete
{
    template<typename K,typename V>
    void test_delete(SkipList<K,V>* list)
    {
         gxh_test_insert::test_insert(list);
         list->delete_element("33");
         list->delete_element("99");
    }
}

namespace gxh_test_display
{
    template<typename K,typename V>
    void test_display(SkipList<K,V>* list)
    {
         gxh_test_insert::test_insert(list);
         list->display_list();
    }
}

namespace gxh_test_search
{
    template<typename K,typename V>
    void test_search(SkipList<K,V>* list)
    {
         gxh_test_insert::test_insert(list);
         string s;
         list->search_element("33",s);
         list->search_element("44",s);
         list->search_element("99",s);
    }
}

namespace gxh_test_dump2file
{
    template<typename K,typename V>
    void test_dump2file(SkipList<K,V>* list)
    {
         gxh_test_insert::test_insert(list);
         list->dump_file();
    }
}

namespace gxh_test_loadfromfile
{
    template<typename K,typename V>
    void test_loadfromfile(SkipList<K,V>* list)
    {
         list->load_file();
         list->display_list();
    }
}
int main(int argc,char* argv[])
{

    SkipList<string,string>* list = new SkipList<string,string>(6);

    /*插入元素测试*/
    // gxh_test_insert::test_insert(list);

    /*删除元素测试*/
    // gxh_test_delete::test_delete(list);

    /*显示元素测试*/
    // gxh_test_display::test_display(list);

    /*查找元素测试*/
    // gxh_test_search::test_search(list);

    /*存储元素到文件测试*/
    // gxh_test_dump2file::test_dump2file(list);

    /*从文件中加载到内存测试*/
    gxh_test_loadfromfile::test_loadfromfile(list);

    return 0;
}