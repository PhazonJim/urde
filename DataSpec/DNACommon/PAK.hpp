#ifndef __DNACOMMON_PAK_HPP__
#define __DNACOMMON_PAK_HPP__

#include "DNACommon.hpp"
#include "boo/ThreadLocalPtr.hpp"

namespace DataSpec
{

/** PAK entry stream reader */
class PAKEntryReadStream : public athena::io::IStreamReader
{
    std::unique_ptr<atUint8[]> m_buf;
    atUint64 m_sz;
    atUint64 m_pos;
public:
    PAKEntryReadStream() {}
    operator bool() const {return m_buf.operator bool();}
    PAKEntryReadStream(const PAKEntryReadStream& other) = delete;
    PAKEntryReadStream(PAKEntryReadStream&& other) = default;
    PAKEntryReadStream& operator=(const PAKEntryReadStream& other) = delete;
    PAKEntryReadStream& operator=(PAKEntryReadStream&& other) = default;
    PAKEntryReadStream(std::unique_ptr<atUint8[]>&& buf, atUint64 sz, atUint64 pos)
    : m_buf(std::move(buf)), m_sz(sz), m_pos(pos)
    {
        if (m_pos >= m_sz)
            LogDNACommon.report(logvisor::Fatal, "PAK stream cursor overrun");
    }
    void seek(atInt64 pos, athena::SeekOrigin origin)
    {
        if (origin == athena::Begin)
            m_pos = pos;
        else if (origin == athena::Current)
            m_pos += pos;
        else if (origin == athena::End)
            m_pos = m_sz + pos;
        if (m_pos > m_sz)
            LogDNACommon.report(logvisor::Fatal, "PAK stream cursor overrun");
    }
    atUint64 position() const {return m_pos;}
    atUint64 length() const {return m_sz;}
    const atUint8* data() const {return m_buf.get();}
    atUint64 readUBytesToBuf(void* buf, atUint64 len)
    {
        atUint64 bufEnd = m_pos + len;
        if (bufEnd > m_sz)
            len -= bufEnd - m_sz;
        memcpy(buf, m_buf.get() + m_pos, len);
        m_pos += len;
        return len;
    }
};

struct UniqueResult
{
    enum class Type
    {
        NotFound,
        Pak,
        Level,
        Area,
        Layer
    } m_type = Type::NotFound;
    const hecl::SystemString* m_levelName = nullptr;
    const hecl::SystemString* m_areaName = nullptr;
    const hecl::SystemString* m_layerName = nullptr;
    UniqueResult() = default;
    UniqueResult(Type tp) : m_type(tp) {}

    template <class PAKBRIDGE>
    void checkEntry(const PAKBRIDGE& pakBridge, const typename PAKBRIDGE::PAKType::Entry& entry);

    hecl::ProjectPath uniquePath(const hecl::ProjectPath& pakPath) const;
};

template <class BRIDGETYPE>
class PAKRouter;

/** Resource extractor type */
template <class PAKBRIDGE>
struct ResExtractor
{
    std::function<bool(PAKEntryReadStream&, const hecl::ProjectPath&)> func_a;
    std::function<bool(const SpecBase&, PAKEntryReadStream&, const hecl::ProjectPath&, PAKRouter<PAKBRIDGE>&,
                       const typename PAKBRIDGE::PAKType::Entry&, bool, hecl::BlenderToken&,
                       std::function<void(const hecl::SystemChar*)>)> func_b;
    const hecl::SystemChar* fileExts[4];
    unsigned weight;
    std::function<void(const SpecBase&, PAKEntryReadStream&, PAKRouter<PAKBRIDGE>&,
                       typename PAKBRIDGE::PAKType::Entry&)> func_name;
};

/** Level hierarchy representation */
template <class IDType>
struct Level
{
    hecl::SystemString name;
    struct Area
    {
        hecl::SystemString name;
        struct Layer
        {
            hecl::SystemString name;
            bool active;
            std::unordered_set<IDType> resources;
        };
        std::vector<Layer> layers;
        std::unordered_set<IDType> resources;
    };
    std::unordered_map<IDType, Area> areas;
};

/** PAKRouter (for detecting shared entry locations) */
template <class BRIDGETYPE>
class PAKRouter : public PAKRouterBase
{
public:
    using PAKType = typename BRIDGETYPE::PAKType;
    using IDType = typename PAKType::IDType;
    using EntryType = typename PAKType::Entry;
    using RigPair = std::pair<IDType, IDType>;
private:
    const std::vector<BRIDGETYPE>* m_bridges = nullptr;
    std::vector<std::pair<hecl::ProjectPath,hecl::ProjectPath>> m_bridgePaths;
    ThreadLocalPtr<void> m_curBridgeIdx;
    const hecl::ProjectPath& m_gameWorking;
    const hecl::ProjectPath& m_gameCooked;
    hecl::ProjectPath m_sharedWorking;
    hecl::ProjectPath m_sharedCooked;
    ThreadLocalPtr<const PAKType> m_pak;
    ThreadLocalPtr<const nod::Node> m_node;
    std::unordered_map<IDType, std::pair<size_t, EntryType*>> m_uniqueEntries;
    std::unordered_map<IDType, std::pair<size_t, EntryType*>> m_sharedEntries;
    std::unordered_map<IDType, RigPair> m_cmdlRigs;
public:
    PAKRouter(const SpecBase& dataSpec, const hecl::ProjectPath& working, const hecl::ProjectPath& cooked)
    : PAKRouterBase(dataSpec),
      m_gameWorking(working), m_gameCooked(cooked),
      m_sharedWorking(working, "Shared"), m_sharedCooked(cooked, "Shared") {}

    void build(std::vector<BRIDGETYPE>& bridges, std::function<void(float)> progress);

    void enterPAKBridge(const BRIDGETYPE& pakBridge);

    hecl::ProjectPath getWorking(const EntryType* entry,
                                 const ResExtractor<BRIDGETYPE>& extractor) const;
    hecl::ProjectPath getWorking(const EntryType* entry) const;
    hecl::ProjectPath getWorking(const IDType& id, bool silenceWarnings=false) const;
    hecl::ProjectPath getCooked(const EntryType* entry) const;
    hecl::ProjectPath getCooked(const IDType& id, bool silenceWarnings=false) const;
    bool isShared()
    {
        const PAKType* pak = m_pak.get();
        return pak ? !pak->m_noShare : false;
    }

    hecl::SystemString getResourceRelativePath(const EntryType& a, const IDType& b) const;

    std::string getBestEntryName(const EntryType& entry) const;
    std::string getBestEntryName(const IDType& entry) const;

    bool extractResources(const BRIDGETYPE& pakBridge, bool force, hecl::BlenderToken& btok,
                          std::function<void(const hecl::SystemChar*, float)> progress);

    const typename BRIDGETYPE::PAKType::Entry* lookupEntry(const IDType& entry,
                                                           const nod::Node** nodeOut=nullptr,
                                                           bool silenceWarnings=false,
                                                           bool currentPAK=false) const;

    template <typename DNA>
    bool lookupAndReadDNA(const IDType& id, DNA& out, bool silenceWarnings=false)
    {
        const nod::Node* node;
        const EntryType* entry = lookupEntry(id, &node, silenceWarnings);
        if (!entry)
            return false;
        PAKEntryReadStream rs = entry->beginReadStream(*node);
        out.read(rs);
        return true;
    }

    const RigPair* lookupCMDLRigPair(const IDType& id) const;

    hecl::ProjectPath getAreaLayerWorking(const IDType& areaId, int layerIdx) const;
    hecl::ProjectPath getAreaLayerWorking(const IDType& areaId, int layerIdx, bool& activeOut) const;
    hecl::ProjectPath getAreaLayerCooked(const IDType& areaId, int layerIdx) const;
    hecl::ProjectPath getAreaLayerCooked(const IDType& areaId, int layerIdx, bool& activeOut) const;
};

}

#endif // __DNACOMMON_PAK_HPP__
