#include <ext/hash_set>
#include <ext/hash_map>

int main(int argc, char **argv) {
  const __gnu_cxx::hash_map<int,int> a;
  const __gnu_cxx::hash_set<int> b;
  a.find(1);
  b.find(1);
}
