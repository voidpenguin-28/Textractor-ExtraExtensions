#pragma once
#include <string>
using namespace std;

const string GPT_MODEL3_5 = "gpt-3.5-turbo";
const string GPT_MODEL4 = "gpt-4";
const string GPT_MODEL4_TURBO = "gpt-4-turbo";
const string GPT_MODEL4_O = "gpt-4o";

//"content": "\n\nHello there, how may I assist you today?",
const string GPT_REQUEST_TEMPLATE = "{\"model\":\"{0}\",\"messages\":[{\"role\":\"system\",\"content\":\"{1}\"},{\"role\":\"user\",\"content\":\"{2}\"}]}";
const string GPT_RESPONSE_MSG_PATTERN = "\"[Cc]ontent\":\\s{0,}\"((?:\\\\\"|[^\"])*)\"";
const string GPT_ERROR_MSG_PATTERN = "\"[Mm]essage\":\\s{0,}\"((?:\\\\\"|[^\"])*)\"";
const string GPT_HTTP_HEADERS = "Content-Type: application/json | Authorization: Bearer {0}";

static constexpr wchar_t ZERO_WIDTH_SPACE = L'\x200b';
