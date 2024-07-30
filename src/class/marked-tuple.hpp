#ifndef MARKED_TUPLE_HPP
#define MARKED_TUPLE_HPP

#include <tuple.hpp>
#include <processing-unit.hpp>
#include <vector>
#include <list>

namespace pp {
 
class MarkedTuple : public Tuple
{
public:
  MarkedTuple(uint32_t* mask, const int& nFields, const int& threshold,
	      ProcessingUnit::Profiler* &profiler, int id = 0);
  ~MarkedTuple();
  void display();

public:
  enum EntryType {
    RULE = 1,
    MARKER = 2,
    MARKED_RULE = 3
  };
  struct Entry {
    uint8_t type;
    pc_rule_t rule;
    uint32_t* key;
    std::vector<Entry*> owners;

    Entry(const pc_rule_t& r, int nFields);
    Entry(Entry* const &entry, int nFields, const field_t& mask);
    ~Entry();
    void refresh(const pc_rule_t& r);
    void addMarker(Entry* const &entry);
    bool setRule(const pc_rule_t& r);
  };
  typedef Entry* entry_t;

public:
  entry_t
  searchEntry(const key_t& matchFields);
  
  virtual bool
  insert(const pc_rule_t& rule) override;

  virtual bool
  erase(const pc_rule_t& rule) override;

  virtual uint64_t
  totalMemory(int bytesPerPointer = 4) const override;

  const pc_rule_t&
  insertMarker(const entry_t& owner);

  void
  leaveMarkers();

  const std::vector<entry_t>&
  getEntries() const { return m_entries; }

  const value_t&
  getHost() const { return m_host; }

  void
  setHost(const value_t& host) { m_host = host; }

  const int&
  getOrder() const { return m_order; }

  void
  setOrder(const int& order) { m_order = order; }
  
  MarkedTuple* prev;
  MarkedTuple* next;
  MarkedTuple* succ;
  MarkedTuple* fail;
protected:
  const int& m_threshold;
  ProcessingUnit::Profiler* &m_profiler;
  bool m_searchTable;
  std::vector<entry_t> m_entries;
  value_t m_host;
  int m_order;
};

} // namespace pp

#endif // MARKED_TUPLE_HPP
