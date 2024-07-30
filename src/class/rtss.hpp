#ifndef RTSS_HPP
#define RTSS_HPP

#include <tss.hpp>
#include <deque>
#include <list>


namespace pp {
 
class RTuple : public OrderedTuple
{
public:
  RTuple(uint32_t* mask, const int& nFields, int id = 0,
	 ProcessingUnit::Profiler* const& profiler = nullptr);
  ~RTuple();
  void display();

public:
  struct List {
    std::deque<pc_rule_t> list_rule;

    List() {};
    List(const pc_rule_t& r);
    ~List();
	
    void addRule(const pc_rule_t& r);
	bool deleteRule(const pc_rule_t& r);
  };
  
  struct XList {
    pc_rule_t p_rule;
	XList *next;
	XList():p_rule(nullptr),next(nullptr){}
	XList(const pc_rule_t &r);
	~XList();
	void addRule(const pc_rule_t &r);
	bool deleteRule(const pc_rule_t &r);
  };
  typedef XList* list_t;
public:
  virtual bool
  insert(const pc_rule_t& rule) override;
  virtual bool
  erase(const pc_rule_t& rule) override;
  
  virtual value_t
  search(const key_t& matchFields) const override;

  virtual uint64_t
  totalMemory(int bytesPerPointer = 4) const override;

  bool check(const pc_rule_t& r);
protected:
  std::vector<list_t> m_lists;
  ProcessingUnit::Profiler* const &m_profiler;
};


class RangeTupleSpaceSearch: public TupleSpaceSearch
{
public:
  static void TEST();
  
  RangeTupleSpaceSearch(int nFields = 2, int maxLength = 32);
  ~RangeTupleSpaceSearch();

public:
  virtual int
  insertRuleConstruction(const pc_rule_t& rule) override;

  virtual uint64_t
  finalizeConstruction() override;
  
  virtual action_t
  matchPacket(const packet_t& packet) override;

  virtual void
  insertRule(const pc_rule_t& rule) override;

  virtual void
  deleteRule(const pc_rule_t& rule) override;
  
public: // profiler
  struct Profiler : public TupleSpaceSearch::Profiler
  {
    int accessedListNodes;
    Profiler(int n) : TupleSpaceSearch::Profiler(n), accessedListNodes(0) {};
    virtual void dump(FILE* file = NULL) override;
  };
  DEFINE_PROFILER
  
public:

  void
  display();

private:
  std::vector<std::vector<uint32_t> > dim;
};
} // namespace pp

#endif // RTSS_HPP
