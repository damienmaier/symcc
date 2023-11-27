
#include "Tracer.h"
#include "Shadow.h"

nlohmann::json Tracer::currentTrace;
nlohmann::json Tracer::symbols;
const std::string Tracer::BACKEND_TRACE_FILE = "/tmp/backend_trace.json";

void Tracer::trace(uintptr_t pc) {

  nlohmann::json newEntry;
  newEntry["pc"] = pc;
  newEntry["memory_to_symbol_mapping"] = nlohmann::json::object();

  for (auto const &[pageAddress, _] : g_shadow_pages) {
    for (auto byteAddress = pageAddress; byteAddress < pageAddress + kPageSize;
         byteAddress++) {
      auto byteExpr = _sym_read_memory((u_int8_t *)byteAddress, 1, true);
      if (byteExpr != nullptr && !byteExpr->isConcrete()) {
        nlohmann::json symbolicAddress;
        symbolicAddress["address"] = byteAddress;
        symbolicAddress["symbol"] = getSymbolID(byteExpr);

        newEntry["memory_to_symbol_mapping"][std::to_string(reinterpret_cast<uintptr_t>(byteAddress))] = getSymbolID(byteExpr);
      }
    }
  }

  currentTrace.push_back(newEntry);
}

void Tracer::writeTraceToDisk() {
  for (auto const &[symbolPtr, _] : getAllocatedExpressions()) {
    recursivelyCollectSymbols(symbolPtr);
  }

  nlohmann::json dataToSave;
  dataToSave["trace"] = currentTrace;
  dataToSave["symbols"] = symbols;

  std::ofstream o(BACKEND_TRACE_FILE);
  o << std::setw(4) << dataToSave << std::endl;
}

void Tracer::recursivelyCollectSymbols(SymExpr symbolPtr) {
  string symbolID = getSymbolID(symbolPtr);
  if (symbols.count(symbolID) > 0) {
    return;
  }

  symbols[symbolID]["kind"] = symbolPtr->kind();
  symbols[symbolID]["args"] = nlohmann::json::array();
  for (int child_i = 0; child_i < symbolPtr->num_children(); child_i++) {
    SymExpr child = symbolPtr->getChild(child_i).get();
    string childID = getSymbolID(child);
    symbols[symbolID]["args"].push_back(childID);
    recursivelyCollectSymbols(child);
  }
}

string Tracer::getSymbolID(SymExpr symbol) {
  return std::to_string(reinterpret_cast<uintptr_t>(symbol));
}