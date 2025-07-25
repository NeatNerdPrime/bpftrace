#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "ast/passes/clang_parser.h"
#include "bpfmap.h"
#include "required_resources.h"
#include "types.h"
#include "util/bpf_funcs.h"

namespace bpftrace {

class BPFtrace;

enum class MessageType {
  // don't forget to update std::ostream& operator<<(std::ostream& out,
  // MessageType type) in output.cpp
  map,
  value,
  hist,
  tseries,
  stats,
  printf,
  time,
  cat,
  join,
  syscall,
  attached_probes,
  lost_events,
  helper_error,
};

std::ostream &operator<<(std::ostream &out, MessageType type);

// Abstract class (interface) for output
// Provides default implementation of some methods for formatting map and
// non-map values into strings.
// All output formatters should extend this class and override (at least) pure
// virtual methods.
class Output {
public:
  explicit Output(ast::CDefinitions &c_definitions,
                  std::ostream &out = std::cout,
                  std::ostream &err = std::cerr)
      : c_definitions_(c_definitions), out_(out), err_(err)
  {
  }
  Output(const Output &) = delete;
  Output &operator=(const Output &) = delete;
  virtual ~Output() = default;

  virtual const ast::CDefinitions &c_definitions() const
  {
    return c_definitions_;
  }
  virtual std::ostream &outputstream() const
  {
    return out_;
  };

  // Write map to output
  virtual void map(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
          &values_by_key) const = 0;
  // Write map histogram to output
  virtual void map_hist(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::map<std::vector<uint8_t>, std::vector<uint64_t>>
          &values_by_key,
      const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
          &total_counts_by_key) const = 0;
  // Write map tseries to output
  virtual void map_tseries(BPFtrace &bpftrace,
                           const BpfMap &map,
                           const TSeriesMap &values_by_key,
                           const std::vector<std::pair<KeyType, EpochType>>
                               &latest_epoch_by_key) const = 0;
  // Write map statistics to output
  virtual void map_stats(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
          &values_by_key) const = 0;
  // Write non-map value to output
  // Ideally, the implementation should use value_to_str to convert a value into
  // a string, format it properly, and print it to out_.
  virtual void value(BPFtrace &bpftrace,
                     const SizedType &ty,
                     std::vector<uint8_t> &value) const = 0;

  virtual void message(MessageType type,
                       const std::string &msg,
                       bool nl = true) const = 0;
  virtual void lost_events(uint64_t lost) const = 0;
  virtual void attached_probes(uint64_t num_probes) const = 0;
  virtual void helper_error(int retcode, const HelperErrorInfo &info) const = 0;
  virtual void benchmark_results(
      const std::map<std::string, uint32_t> &results) const = 0;

protected:
  ast::CDefinitions &c_definitions_;
  std::ostream &out_;
  std::ostream &err_;
  void hist_prepare(const std::vector<uint64_t> &values,
                    int &min_index,
                    int &max_index,
                    int &max_value) const;
  void lhist_prepare(const std::vector<uint64_t> &values,
                     int min,
                     int max,
                     int step,
                     int &max_index,
                     int &max_value,
                     int &buckets,
                     int &start_value,
                     int &end_value) const;
  std::string get_helper_error_msg(libbpf::bpf_func_id func_id,
                                   int retcode) const;
  // Convert a log2 histogram into string
  virtual std::string hist_to_str(const std::vector<uint64_t> &values,
                                  uint32_t div,
                                  uint32_t k) const = 0;
  // Convert a linear histogram into string
  virtual std::string lhist_to_str(const std::vector<uint64_t> &values,
                                   int min,
                                   int max,
                                   int step) const = 0;
  // Convert a time series into string
  virtual std::string tseries_to_str(BPFtrace &bpftrace,
                                     const TSeries &values,
                                     EpochType last_epoch,
                                     uint64_t interval_ns,
                                     uint64_t num_intervals,
                                     const SizedType &value_type) const = 0;

  // Convert map into string
  // Default behaviour: format each (key, value) pair using output-specific
  // methods and join them into a single string
  virtual void map_contents(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
          &values_by_key) const;
  // Convert map histogram into string
  // Default behaviour: format each (key, hist) pair using output-specific
  // methods and join them into a single string
  virtual void map_hist_contents(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::map<std::vector<uint8_t>, std::vector<uint64_t>>
          &values_by_key,
      const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
          &total_counts_by_key) const;
  // Convert map tseries into string
  // Default behaviour: format each (key, tseries) pair using output-specific
  // methods and join them into a single string
  virtual void map_tseries_contents(
      BPFtrace &bpftrace,
      const BpfMap &map,
      const TSeriesMap &values_by_key,
      const std::vector<std::pair<KeyType, EpochType>> &latest_epoch_by_key)
      const;
  // Convert map statistics into string
  // Default behaviour: format each (key, stats) pair using output-specific
  // methods and join them into a single string
  virtual void map_stats_contents(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
          &values_by_key) const;
  // Convert map key to string
  virtual std::string map_key_to_str(BPFtrace &bpftrace,
                                     const BpfMap &map,
                                     const std::vector<uint8_t> &key) const = 0;
  // Properly join map key and value strings
  virtual void map_key_val(const SizedType &map_type,
                           const std::string &key,
                           const std::string &val) const = 0;
  // Delimiter to join (properly formatted) map elements into a single string
  virtual void map_elem_delim(const SizedType &map_type) const = 0;
  // Convert non-map value into string
  // This method should properly handle all non-map value types.
  // Aggregate types (array, struct, tuple) are formatted using output-specific
  // methods.
  virtual std::string value_to_str(BPFtrace &bpftrace,
                                   const SizedType &type,
                                   const std::vector<uint8_t> &value,
                                   bool is_per_cpu = false,
                                   uint32_t div = 1,
                                   bool is_map_key = false) const;
  virtual std::string map_key_str(BPFtrace &bpftrace,
                                  const SizedType &arg,
                                  const std::vector<uint8_t> &data) const;
  // Convert an array to string
  // Default behaviour: [elem1, elem2, ...]
  virtual std::string array_to_str(const std::vector<std::string> &elems) const;
  // Convert a struct to string
  // Default behaviour: { elem1, elem2, ... }
  // elems are expected to be properly formatted
  virtual std::string struct_to_str(
      const std::vector<std::string> &elems) const;
  // Convert struct field (given by its name and value) into string
  virtual std::string field_to_str(const std::string &name,
                                   const std::string &value) const = 0;
  // Convert tuple to string
  virtual std::string tuple_to_str(const std::vector<std::string> &elems,
                                   bool is_map_key = false) const = 0;
  // Convert a vector of (key, value) pairs into string
  // Used for properly formatting map statistics
  virtual std::string key_value_pairs_to_str(
      std::vector<std::pair<std::string, std::string>> &keyvals) const = 0;
};

class TextOutput : public Output {
public:
  explicit TextOutput(ast::CDefinitions &c_definitions,
                      std::ostream &out = std::cout,
                      std::ostream &err = std::cerr)
      : Output(c_definitions, out, err)
  {
  }

  void map(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
          &values_by_key) const override;
  void map_hist(BPFtrace &bpftrace,
                const BpfMap &map,
                uint32_t top,
                uint32_t div,
                const std::map<std::vector<uint8_t>, std::vector<uint64_t>>
                    &values_by_key,
                const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
                    &total_counts_by_key) const override;
  void map_tseries(BPFtrace &bpftrace,
                   const BpfMap &map,
                   const TSeriesMap &values_by_key,
                   const std::vector<std::pair<KeyType, EpochType>>
                       &latest_epoch_by_key) const override;
  void map_stats(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
          &values_by_key) const override;
  void value(BPFtrace &bpftrace,
             const SizedType &ty,
             std::vector<uint8_t> &value) const override;

  void message(MessageType type,
               const std::string &msg,
               bool nl = true) const override;
  void lost_events(uint64_t lost) const override;
  void attached_probes(uint64_t num_probes) const override;
  void helper_error(int retcode, const HelperErrorInfo &info) const override;
  void benchmark_results(
      const std::map<std::string, uint32_t> &results) const override;

protected:
  std::string value_to_str(BPFtrace &bpftrace,
                           const SizedType &type,
                           const std::vector<uint8_t> &value,
                           bool is_per_cpu,
                           uint32_t div,
                           bool is_map_key = false) const override;
  static std::string hist_index_label(uint32_t index, uint32_t k);
  static std::string lhist_index_label(int number, int step);
  std::string hist_to_str(const std::vector<uint64_t> &values,
                          uint32_t div,
                          uint32_t k) const override;
  std::string lhist_to_str(const std::vector<uint64_t> &values,
                           int min,
                           int max,
                           int step) const override;
  std::string tseries_to_str(BPFtrace &bpftrace,
                             const TSeries &values,
                             EpochType last_epoch,
                             uint64_t interval_ns,
                             uint64_t num_intervals,
                             const SizedType &value_type) const override;

  std::string map_key_to_str(BPFtrace &bpftrace,
                             const BpfMap &map,
                             const std::vector<uint8_t> &key) const override;
  void map_key_val(const SizedType &map_type,
                   const std::string &key,
                   const std::string &val) const override;
  void map_elem_delim(const SizedType &map_type) const override;
  std::string field_to_str(const std::string &name,
                           const std::string &value) const override;
  std::string tuple_to_str(const std::vector<std::string> &elems,
                           bool is_map_key) const override;
  std::string key_value_pairs_to_str(
      std::vector<std::pair<std::string, std::string>> &keyvals) const override;
};

class JsonOutput : public Output {
public:
  explicit JsonOutput(ast::CDefinitions &c_definitions,
                      std::ostream &out = std::cout,
                      std::ostream &err = std::cerr)
      : Output(c_definitions, out, err)
  {
  }

  void map(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
          &values_by_key) const override;
  void map_hist(BPFtrace &bpftrace,
                const BpfMap &map,
                uint32_t top,
                uint32_t div,
                const std::map<std::vector<uint8_t>, std::vector<uint64_t>>
                    &values_by_key,
                const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
                    &total_counts_by_key) const override;
  void map_tseries(BPFtrace &bpftrace,
                   const BpfMap &map,
                   const TSeriesMap &values_by_key,
                   const std::vector<std::pair<KeyType, EpochType>>
                       &latest_epoch_by_key) const override;
  void map_stats(
      BPFtrace &bpftrace,
      const BpfMap &map,
      uint32_t top,
      uint32_t div,
      const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
          &values_by_key) const override;
  void value(BPFtrace &bpftrace,
             const SizedType &ty,
             std::vector<uint8_t> &value) const override;

  void message(MessageType type,
               const std::string &msg,
               bool nl = true) const override;
  void message(MessageType type,
               const std::string &field,
               uint64_t value) const;
  void lost_events(uint64_t lost) const override;
  void attached_probes(uint64_t num_probes) const override;
  void helper_error(int retcode, const HelperErrorInfo &info) const override;
  void benchmark_results(
      const std::map<std::string, uint32_t> &results) const override;

private:
  std::string json_escape(const std::string &str) const;

protected:
  std::string value_to_str(BPFtrace &bpftrace,
                           const SizedType &type,
                           const std::vector<uint8_t> &value,
                           bool is_per_cpu,
                           uint32_t div,
                           bool is_map_key) const override;
  std::string hist_to_str(const std::vector<uint64_t> &values,
                          uint32_t div,
                          uint32_t k) const override;
  std::string lhist_to_str(const std::vector<uint64_t> &values,
                           int min,
                           int max,
                           int step) const override;
  std::string tseries_to_str(BPFtrace &bpftrace,
                             const TSeries &values,
                             EpochType last_epoch,
                             uint64_t interval_ns,
                             uint64_t num_intervals,
                             const SizedType &value_type) const override;

  std::string map_key_to_str(BPFtrace &bpftrace,
                             const BpfMap &map,
                             const std::vector<uint8_t> &key) const override;
  void map_key_val(const SizedType &map_type,
                   const std::string &key,
                   const std::string &val) const override;
  void map_elem_delim(const SizedType &map_type) const override;
  std::string field_to_str(const std::string &name,
                           const std::string &value) const override;
  std::string tuple_to_str(const std::vector<std::string> &elems,
                           bool is_map_key) const override;
  std::string key_value_pairs_to_str(
      std::vector<std::pair<std::string, std::string>> &keyvals) const override;
};

} // namespace bpftrace
