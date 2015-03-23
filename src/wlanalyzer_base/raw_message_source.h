#ifndef RAW_MESSAGE_SOURCE_H
#define RAW_MESSAGE_SOURCE_H

#include <tr1/memory>

namespace WlAnalyzer {

using std::tr1::shared_ptr;

class RawMessage;
class RawMessageSink;

class RawMessageSource {
public:
    virtual ~RawMessageSource() {}
    void setSink(const shared_ptr<RawMessageSink> &sink);

protected:
    void sendMessage(const shared_ptr<RawMessage> &message);

private:
    shared_ptr<RawMessageSink> sink_;
};

}   // namespace WlAnalyzer

#endif // RAW_MESSAGE_SOURCE_H