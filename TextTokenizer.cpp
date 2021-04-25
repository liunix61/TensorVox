#include "TextTokenizer.h"
#include "ext/ZCharScanner.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
const std::vector<std::string> first14 = { "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen" };
const std::vector<std::string> prefixes = { "twen", "thir", "for", "fif", "six", "seven", "eigh", "nine" };

// Punctuation, this gets auto-converted to SIL
const std::u32string punctuation_f = U",.-;";

// For Tacotron2, including question and other marks
const std::u32string punctuation_tac = U",.;¡!¿?:";


using namespace std;

void TextTokenizer::SetAllowedChars(const std::string &value)
{
    AllowedChars = VoxUtil::StrToU32(value);
}

string TextTokenizer::IntToStr(int number)
{
    if (number < 0)
    {
        return "minus " + IntToStr(-number);
	}
	if (number <= 14)
		return first14.at(number);
	if (number < 20)
		return prefixes.at(number - 12) + "teen";
	if (number < 100) {
		unsigned int remainder = number - (static_cast<int>(number / 10) * 10);
		return prefixes.at(number / 10 - 2) + (0 != remainder ? "ty " + IntToStr(remainder) : "ty");
	}
	if (number < 1000) {
		unsigned int remainder = number - (static_cast<int>(number / 100) * 100);
		return first14.at(number / 100) + (0 != remainder ? " hundred " + IntToStr(remainder) : " hundred");
	}
	if (number < 1000000) {
		unsigned int thousands = static_cast<int>(number / 1000);
		unsigned int remainder = number - (thousands * 1000);
		return IntToStr(thousands) + (0 != remainder ? " thousand " + IntToStr(remainder) : " thousand");
	}
	if (number < 1000000000) {
		unsigned int millions = static_cast<int>(number / 1000000);
		unsigned int remainder = number - (millions * 1000000);
		return IntToStr(millions) + (0 != remainder ? " million " + IntToStr(remainder) : " million");
	}
	throw std::out_of_range("inttostr() value too large");
}


vector<string> TextTokenizer::ExpandNumbers(const std::vector<std::string>& SpaceTokens)
{
	vector<string> RetVec;
	RetVec.reserve(SpaceTokens.size());

	for (auto& Token : SpaceTokens) {
		char* p;
		long converted = strtol(Token.c_str(), &p, 10);
		if (*p) {
			RetVec.push_back(Token);
		}
		else {
			if (converted > 1000000000)
				continue;

			string IntStr = IntToStr((int)converted);
			ZStringDelimiter DelInt(IntStr);
			DelInt.AddDelimiter(" ");

			std::vector<std::string> NumToks = DelInt.GetTokens();

			// If a number results in one word the delimiter may not add it.
			if (NumToks.empty())
				NumToks.push_back(IntStr);

			for (const auto& NumTok : NumToks)
				RetVec.push_back(NumTok);
			

		}
	}

	return RetVec;
	
}

TextTokenizer::TextTokenizer()
{
}

TextTokenizer::~TextTokenizer()
{
}


vector<string> TextTokenizer::Tokenize(const std::string & InTxt,ETTSLanguage::Enum Language,bool IsTacotron)
{
	vector<string> ProcessedTokens;




	ZStringDelimiter Delim(InTxt);
	Delim.AddDelimiter(" ");

    vector<string> DelimitedTokens = Delim.GetTokens();



	// Single word handler
    if (!Delim.szTokens())
        DelimitedTokens.push_back(InTxt);

    if (Language == ETTSLanguage::English)
        DelimitedTokens = ExpandNumbers(DelimitedTokens);

    std::u32string punctuation = punctuation_f;

    if (IsTacotron)
        punctuation = punctuation_tac;




	// We know that the new vector is going to be at least this size so we reserve
	ProcessedTokens.reserve(DelimitedTokens.size());

	/*
	In this step we go through the string and only allow qualified character to pass through.
	*/
    for (size_t TokCtr = 0; TokCtr < DelimitedTokens.size();TokCtr++)
    {
        // We are now using U32string because it's guaranteed to be 1 character = 1 element
        const auto& tok = VoxUtil::StrToU32(DelimitedTokens[TokCtr]);
        std::u32string AppTok = U"";


        if (tok.find(U"@") != string::npos)
        {

            ProcessedTokens.push_back(VoxUtil::U32ToStr(tok));
            continue;

        }

		for (size_t s = 0;s < tok.size();s++)
		{


            if (AllowedChars.find(tok[s]) != std::u32string::npos)
                AppTok += tok[s];


			// Punctuation handler
            // This time we explicitly add a token to the vector
            if (punctuation.find(tok[s]) != std::u32string::npos) {
				// First, if the assembled string isn't empty, we add it in its current state
				// Otherwise, the SIL could end up appearing before the word.

                if (!AppTok.empty()) {
                    ProcessedTokens.push_back(VoxUtil::U32ToStr(AppTok));

                    AppTok = U"";
				}

                if (IsTacotron){

                    // Double at-symbol is handled later
                    AppTok += U"@@";
                    AppTok += tok[s];

                }
                else{
                    AppTok = U"@SIL";
                }

                ProcessedTokens.push_back(VoxUtil::U32ToStr(AppTok));
                AppTok = U"";
                continue;

			}






		}
        if (!AppTok.empty())
        {
            ProcessedTokens.push_back(VoxUtil::U32ToStr(AppTok));
            AppTok = U"";


        }

	}
	// Prevent out of range error if the user inputs one word
	if (ProcessedTokens.size() > 1) 
	{
		if (ProcessedTokens[ProcessedTokens.size() - 1] == "SIL")
			ProcessedTokens.pop_back();
	}


	return ProcessedTokens;
}
