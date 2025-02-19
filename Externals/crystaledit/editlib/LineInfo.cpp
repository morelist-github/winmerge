/** 
 * @file  LineInfo.cpp
 *
 * @brief Implementation of LineInfo class.
 */

#include "stdafx.h"
#include "LineInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 @brief Constructor.
 */
LineInfo::LineInfo()
: m_pcLine(nullptr)
, m_nLength(0)
, m_nMax(0)
, m_nEolChars(0)
, m_dwFlags(0)
, m_dwRevisionNumber(0)
{
};

/**
 * @brief Clear item.
 * Frees buffer, sets members to initial values.
 */
void LineInfo::Clear()
{
  if (m_pcLine != nullptr)
    {
      delete[] m_pcLine;
      m_pcLine = nullptr;
      m_nLength = 0;
      m_nMax = 0;
      m_nEolChars = 0;
      m_dwFlags = 0;
      m_dwRevisionNumber = 0;
    }
}

/**
 * @brief Free reserved memory.
 * Frees reserved memory, but does not clear flags.
 */
void LineInfo::FreeBuffer()
{
  if (m_pcLine != nullptr)
    {
      delete[] m_pcLine;
      m_pcLine = nullptr;
      m_nLength = 0;
      m_nMax = 0;
      m_nEolChars = 0;
    }
}

/**
 * @brief Create a line.
 * @param [in] pszLine Line data.
 * @param [in] nLength Line length.
 */
void LineInfo::Create(const tchar_t* pszLine, size_t nLength)
{
  if (nLength == 0)
    {
      CreateEmpty();
      return;
    }

  ASSERT (nLength <= INT_MAX);		// assert "positive int"
  m_nLength = nLength;
  m_nMax = ALIGN_BUF_SIZE (m_nLength + 1);
  ASSERT (m_nMax < INT_MAX);
  ASSERT (m_nMax >= m_nLength + 1);
  if (m_pcLine != nullptr)
    delete[] m_pcLine;
  m_pcLine = new tchar_t[m_nMax];
  ZeroMemory(m_pcLine, m_nMax * sizeof(tchar_t));
  const size_t dwLen = sizeof (tchar_t) * m_nLength;
  CopyMemory (m_pcLine, pszLine, dwLen);
  m_pcLine[m_nLength] = '\0';

  int nEols = 0;
  if (nLength > 1 && IsDosEol(&pszLine[nLength - 2]))
    nEols = 2;
  else if (IsEol(pszLine[nLength - 1]))
    nEols = 1;
  ASSERT (static_cast<size_t>(nEols) <= m_nLength);
  m_nLength -= nEols;
  m_nEolChars = nEols;
}

/**
 * @brief Create an empty line.
 */
void LineInfo::CreateEmpty()
{
  m_nLength = 0;
  m_nEolChars = 0;
  m_nMax = ALIGN_BUF_SIZE (m_nLength + 1);
  delete [] m_pcLine;
  m_pcLine = new tchar_t[m_nMax];
  ZeroMemory(m_pcLine, m_nMax * sizeof(tchar_t));
}

/**
 * @brief Append a text to the line.
 * @param [in] pszChars String to append to the line.
 * @param [in] nLength Length of the string to append.
 */
void LineInfo::Append(const tchar_t* pszChars, size_t nLength, bool bDetectEol)
{
  ASSERT (nLength <= INT_MAX);		// assert "positive int"
  size_t nBufNeeded = m_nLength + m_nEolChars + nLength + 1;
  if (nBufNeeded > m_nMax)
    {
      m_nMax = ALIGN_BUF_SIZE (nBufNeeded);
      ASSERT (m_nMax < INT_MAX);
      ASSERT (m_nMax >= m_nLength + nLength);
      tchar_t *pcNewBuf = new tchar_t[m_nMax];
      if (FullLength() > 0)
        memcpy (pcNewBuf, m_pcLine, sizeof (tchar_t) * (FullLength() + 1));
      delete[] m_pcLine;
      m_pcLine = pcNewBuf;
    }

  memcpy (m_pcLine + m_nLength + m_nEolChars, pszChars, sizeof (tchar_t) * nLength);
  m_nLength += nLength + m_nEolChars;
  m_pcLine[m_nLength] = '\0';

  if (!bDetectEol)
    return;

  // Did line gain eol ? (We asserted above that it had none at start)
   if (nLength > 1 && IsDosEol(&m_pcLine[m_nLength - 2]))
     {
       m_nEolChars = 2;
     }
   else if (LineInfo::IsEol(m_pcLine[m_nLength - 1]))
      {
       m_nEolChars = 1;
      }
   ASSERT (static_cast<size_t>(m_nEolChars) <= m_nLength);
   m_nLength -= m_nEolChars;
   ASSERT (m_nLength + m_nEolChars <= m_nMax);
}

/**
 * @brief Has the line EOL?
 * @return true if the line has EOL bytes.
 */
bool LineInfo::HasEol() const
{
  if (m_nEolChars)
    return true;
  else
    return false;
}

/**
 * @brief Get line's EOL bytes.
 * @return EOL bytes, or `nullptr` if no EOL bytes.
 */
const tchar_t* LineInfo::GetEol() const
{
  if (HasEol())
    return &m_pcLine[Length()];
  else
    return nullptr;
}

/**
 * @brief Change line's EOL.
 * @param [in] lpEOL New EOL bytes.
 * @return true if succeeded, false if failed (nothing to change).
 */
bool LineInfo::ChangeEol(const tchar_t* lpEOL)
{
  const int nNewEolChars = (int) tc::tcslen(lpEOL);

  // Check if we really are changing EOL.
  if (nNewEolChars == m_nEolChars)
    if (tc::tcscmp(m_pcLine + Length(), lpEOL) == 0)
      return false;

  size_t nBufNeeded = m_nLength + nNewEolChars+1;
  ASSERT (nBufNeeded < INT_MAX);
  if (nBufNeeded > m_nMax)
    {
      m_nMax = ALIGN_BUF_SIZE (nBufNeeded);
      ASSERT (m_nMax >= nBufNeeded);
      tchar_t *pcNewBuf = new tchar_t[m_nMax];
      if (FullLength() > 0)
        memcpy (pcNewBuf, m_pcLine, sizeof (tchar_t) * (FullLength() + 1));
      delete[] m_pcLine;
      m_pcLine = pcNewBuf;
    }
  
  // copy also the 0 to zero-terminate the line
  memcpy (m_pcLine + m_nLength, lpEOL, sizeof (tchar_t) * (nNewEolChars + 1));
  m_nEolChars = nNewEolChars;
  return true;
}

/**
 * @brief Delete part of the line.
 * @param [in] nStartChar Start position for removal.
 * @param [in] nEndChar End position for removal.
 */
void LineInfo::Delete(size_t nStartChar, size_t nEndChar)
{
  if (nEndChar < Length() || m_nEolChars)
    {
      // preserve characters after deleted range by shifting up
      memcpy (m_pcLine + nStartChar, m_pcLine + nEndChar,
              sizeof (tchar_t) * (FullLength() - nEndChar));
    }
  size_t nDelete = (nEndChar - nStartChar);
  if (nDelete <= m_nLength)
    {
      m_nLength -= nDelete;
    }
  else
    {
      ASSERT( (m_nLength + m_nEolChars) <= nDelete );
      nDelete -= m_nLength;
      m_nLength = 0;
      m_nEolChars -= static_cast<int>(nDelete);
    }
  ASSERT (m_nLength <= INT_MAX);		// assert "positive int"
  if (m_pcLine != nullptr)
    m_pcLine[FullLength()] = '\0';
}

/**
 * @brief Delete line contents from given index to the end.
 * @param [in] Index of first character to remove.
 */
void LineInfo::DeleteEnd(size_t nStartChar)
{
  m_nLength = nStartChar;
  ASSERT (m_nLength <= INT_MAX);		// assert "positive int"
  if (m_pcLine != nullptr)
    m_pcLine[nStartChar] = 0;
  m_nEolChars = 0;
}

/**
 * @brief Copy contents from another LineInfo item.
 * @param [in] li Item to copy.
 */
void LineInfo::CopyFrom(const LineInfo &li)
{
  delete [] m_pcLine;
  m_pcLine = new tchar_t[li.m_nMax];
  memcpy(m_pcLine, li.m_pcLine, li.m_nMax * sizeof(tchar_t));
}

/**
 * @brief Remove EOL from line.
 */
void LineInfo::RemoveEol()
{
  if (HasEol())
  {
    m_pcLine[m_nLength] = '\0';
    m_nEolChars = 0;
  }
}

/**
 * @brief Get line contents.
 * @param [in] index Index of first character to get.
 * @note Make a copy from returned string, as it can get reallocated.
 */
const tchar_t* LineInfo::GetLine(size_t index) const
{
  return &m_pcLine[index];
}
