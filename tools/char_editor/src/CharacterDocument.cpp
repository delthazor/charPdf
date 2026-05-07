#include "CharacterDocument.h"

namespace CharEditor
{

CharacterDocument::CharacterDocument() : doc(nlohmann::ordered_json::object()), dirty(false) {}

CharacterDocument::CharacterDocument(nlohmann::ordered_json d) : doc(std::move(d)), dirty(false) {}

}

