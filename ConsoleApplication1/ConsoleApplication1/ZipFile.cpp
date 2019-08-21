#include "pch.h"
#include "ZipFile.h"

std::optional<int> ZipFile::Find(const std::string & path) const
{
	std::string lowerCase = path;
	std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), (int(*)(int) std::tolower);
	ZipContentsMap::const_iterator i = m_ZipContentsMap.find(lowerCase);
	if (i == m_ZipContentsMap.end())
		return std::nullopt;

	return i->second;
}
