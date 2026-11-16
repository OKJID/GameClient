// MacOSRichMessageBox.cpp — the game's OK message box whose body understands **bold**, `monospace` and {gold}color{/}

#include "MacOSRichMessageBox.h"

#include "Common/NameKeyGenerator.h"
#include "GameClient/Color.h"
#include "GameClient/DisplayString.h"
#include "GameClient/DisplayStringManager.h"
#include "GameClient/Gadget.h"
#include "GameClient/GameFont.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/MessageBox.h"

#include <algorithm>
#include <cwchar>
#include <map>
#include <vector>

namespace
{
	const char* const kMessageBodyName = "MessageBox.wnd:StaticTextMessage";
	const char* const kMonospaceFontName = "Menlo";
	const Int kWrapInset = 10;
	const Int kMinPointSize = 8;
	const WideChar kBoldMarker = L'*';
	const WideChar kMonospaceMarker = L'`';
	const WideChar kColorTagOpen = L'{';
	const WideChar kColorTagClose = L'}';
	const WideChar* const kColorReset = L"/";

	struct RunStyle
	{
		bool bold = false;
		bool monospace = false;
		bool hasColor = false;
		Color color = 0;

		bool operator==( const RunStyle& other ) const
		{
			return bold == other.bold
				&& monospace == other.monospace
				&& hasColor == other.hasColor
				&& color == other.color;
		}
	};

	struct Piece
	{
		UnicodeString text;
		RunStyle style;
		bool spaceBefore = false;
		Int lineBreaksBefore = 0;
	};

	bool findNamedColor( const UnicodeString& name, Color* color )
	{
		struct NamedColor
		{
			const WideChar* name;
			Color color;
		};

		static const NamedColor kPalette[] = {
			{ L"gold", GameMakeColor( 255, 200, 40, 255 ) },
			{ L"green", GameMakeColor( 120, 220, 100, 255 ) },
			{ L"red", GameMakeColor( 240, 90, 80, 255 ) },
		};

		for ( const NamedColor& entry : kPalette )
		{
			if ( wcscmp( name.str(), entry.name ) == 0 )
			{
				*color = entry.color;
				return true;
			}
		}
		return false;
	}

	class MarkupParser
	{
	public:
		explicit MarkupParser( const UnicodeString& markup )
			: m_text( markup.str() )
			, m_length( markup.getLength() )
		{
		}

		std::vector<Piece> parse()
		{
			while ( m_index < m_length )
			{
				consumeNext();
			}
			flushWord();
			return m_pieces;
		}

	private:
		void consumeNext()
		{
			const WideChar character = m_text[m_index];
			if ( character == L'\n' )
			{
				startNewLine();
				return;
			}

			if ( character == L' ' )
			{
				flushWord();
				m_pendingSpace = true;
				++m_index;
				return;
			}

			if ( isBoldMarker() )
			{
				flushWord();
				m_style.bold = !m_style.bold;
				m_index += 2;
				return;
			}

			if ( character == kMonospaceMarker )
			{
				flushWord();
				m_style.monospace = !m_style.monospace;
				++m_index;
				return;
			}

			if ( character == kColorTagOpen && consumeColorTag() )
			{
				return;
			}

			m_word.concat( character );
			++m_index;
		}

		void startNewLine()
		{
			flushWord();
			++m_pendingLineBreaks;
			m_pendingSpace = false;
			++m_index;
		}

		bool isBoldMarker() const
		{
			return m_index + 1 < m_length
				&& m_text[m_index] == kBoldMarker
				&& m_text[m_index + 1] == kBoldMarker;
		}

		bool consumeColorTag()
		{
			const Int close = findColorTagClose();
			if ( close < 0 )
			{
				return false;
			}

			UnicodeString name;
			name.set( m_text + m_index + 1, close - m_index - 1 );

			RunStyle style = m_style;
			if ( wcscmp( name.str(), kColorReset ) == 0 )
			{
				style.hasColor = false;
			}
			else if ( findNamedColor( name, &style.color ) )
			{
				style.hasColor = true;
			}
			else
			{
				return false;
			}

			flushWord();
			m_style = style;
			m_index = close + 1;
			return true;
		}

		Int findColorTagClose() const
		{
			for ( Int index = m_index + 1; index < m_length; ++index )
			{
				const WideChar character = m_text[index];
				if ( character == kColorTagClose )
				{
					return index;
				}

				if ( character == L' ' || character == L'\n' )
				{
					return -1;
				}
			}
			return -1;
		}

		void flushWord()
		{
			if ( m_word.isEmpty() )
			{
				return;
			}

			Piece piece;
			piece.text = m_word;
			piece.style = m_style;
			piece.spaceBefore = m_pendingSpace;
			piece.lineBreaksBefore = m_pendingLineBreaks;
			m_pieces.push_back( piece );

			m_word.clear();
			m_pendingSpace = false;
			m_pendingLineBreaks = 0;
		}

		const WideChar* m_text;
		Int m_length;
		Int m_index = 0;
		RunStyle m_style;
		UnicodeString m_word;
		bool m_pendingSpace = false;
		Int m_pendingLineBreaks = 0;
		std::vector<Piece> m_pieces;
	};
}

class MacOSRichText
{
public:
	explicit MacOSRichText( const UnicodeString& markup )
		: m_pieces( MarkupParser( markup ).parse() )
	{
	}

	~MacOSRichText()
	{
		releaseStrings();
	}

	void draw( GameWindow* window )
	{
		ICoord2D origin;
		ICoord2D size;
		window->winGetScreenPosition( &origin.x, &origin.y );
		window->winGetSize( &size.x, &size.y );

		const Int wrapWidth = size.x - kWrapInset;
		const Int maxHeight = size.y - kWrapInset;
		GameFont* baseFont = window->winGetFont();
		if ( wrapWidth != m_wrapWidth || maxHeight != m_maxHeight || baseFont != m_baseFont )
		{
			layout( baseFont, wrapWidth, maxHeight );
		}

		drawLines( window, origin, size );
	}

private:
	struct Segment
	{
		UnicodeString text;
		RunStyle style;
		GameFont* font = nullptr;
		Int gapBefore = 0;
		Int width = 0;
		DisplayString* string = nullptr;
	};

	struct Line
	{
		std::vector<Segment> segments;
		Int width = 0;
		Int height = 0;
	};

	void layout( GameFont* baseFont, Int wrapWidth, Int maxHeight )
	{
		releaseStrings();
		m_lines.clear();
		m_baseFont = baseFont;
		m_wrapWidth = wrapWidth;
		m_maxHeight = maxHeight;

		if ( baseFont == nullptr || TheDisplayStringManager == nullptr || TheFontLibrary == nullptr )
		{
			return;
		}

		m_measure = TheDisplayStringManager->newDisplayString();
		for ( m_pointSize = baseFont->pointSize; ; --m_pointSize )
		{
			placeAllPieces();
			if ( totalHeight() <= maxHeight || m_pointSize <= kMinPointSize )
			{
				break;
			}
		}
		TheDisplayStringManager->freeDisplayString( m_measure );
		m_measure = nullptr;

		createStrings();
	}

	void placeAllPieces()
	{
		m_lines.clear();
		m_lines.push_back( Line() );
		for ( const Piece& piece : m_pieces )
		{
			placePiece( piece );
		}

		const Int paragraphGap = fontFor( RunStyle() )->height / 2;
		for ( Line& line : m_lines )
		{
			line.height = line.segments.empty() ? paragraphGap : tallestFont( line );
		}
	}

	static Int tallestFont( const Line& line )
	{
		Int height = 0;
		for ( const Segment& segment : line.segments )
		{
			height = std::max( height, segment.font->height );
		}
		return height;
	}

	void placePiece( const Piece& piece )
	{
		for ( Int index = 0; index < piece.lineBreaksBefore; ++index )
		{
			m_lines.push_back( Line() );
		}

		GameFont* font = fontFor( piece.style );
		if ( appendToLastSegment( m_lines.back(), piece, font ) )
		{
			return;
		}

		Line& line = m_lines.back();
		const Int gap = ( piece.spaceBefore && !line.segments.empty() ) ? spaceWidth( font ) : 0;
		const Int width = measureWidth( piece.text, font );
		if ( !line.segments.empty() && line.width + gap + width > m_wrapWidth )
		{
			m_lines.push_back( Line() );
			addSegment( m_lines.back(), piece, font, 0, width );
			return;
		}

		addSegment( line, piece, font, gap, width );
	}

	bool appendToLastSegment( Line& line, const Piece& piece, GameFont* font )
	{
		if ( line.segments.empty() )
		{
			return false;
		}

		Segment& last = line.segments.back();
		if ( !( last.style == piece.style ) )
		{
			return false;
		}

		UnicodeString joined = last.text;
		if ( piece.spaceBefore )
		{
			joined.concat( L' ' );
		}
		joined.concat( piece.text );

		const Int joinedWidth = measureWidth( joined, font );
		if ( line.width - last.width + joinedWidth > m_wrapWidth )
		{
			return false;
		}

		line.width += joinedWidth - last.width;
		last.text = joined;
		last.width = joinedWidth;
		return true;
	}

	static void addSegment( Line& line, const Piece& piece, GameFont* font, Int gap, Int width )
	{
		Segment segment;
		segment.text = piece.text;
		segment.style = piece.style;
		segment.font = font;
		segment.gapBefore = gap;
		segment.width = width;
		line.segments.push_back( segment );
		line.width += gap + width;
	}

	void createStrings()
	{
		for ( Line& line : m_lines )
		{
			for ( Segment& segment : line.segments )
			{
				segment.string = TheDisplayStringManager->newDisplayString();
				segment.string->setFont( segment.font );
				segment.string->setText( segment.text );
			}
		}
	}

	GameFont* fontFor( const RunStyle& style ) const
	{
		const AsciiString name = style.monospace ? AsciiString( kMonospaceFontName ) : m_baseFont->nameString;
		const Bool bold = style.bold || m_baseFont->bold;
		GameFont* font = TheFontLibrary->getFont( name, m_pointSize, bold );
		return font != nullptr ? font : m_baseFont;
	}

	Int measureWidth( const UnicodeString& text, GameFont* font ) const
	{
		m_measure->setFont( font );
		m_measure->setText( text );
		Int width = 0;
		Int height = 0;
		m_measure->getSize( &width, &height );
		return width;
	}

	Int spaceWidth( GameFont* font ) const
	{
		return measureWidth( UnicodeString( L"x x" ), font ) - measureWidth( UnicodeString( L"xx" ), font );
	}

	void drawLines( GameWindow* window, const ICoord2D& origin, const ICoord2D& size )
	{
		TextData* textData = (TextData*)window->winGetUserData();
		if ( textData == nullptr || m_lines.empty() )
		{
			return;
		}

		const UnsignedInt status = window->winGetStatus();
		const bool isEnabled = BitIsSet( status, WIN_STATUS_ENABLED );
		const Color textColor = isEnabled ? window->winGetEnabledTextColor() : window->winGetDisabledTextColor();
		const Color dropColor = isEnabled ? window->winGetEnabledTextBorderColor() : window->winGetDisabledTextBorderColor();

		const Int blockWidth = widestLine();
		const Int blockHeight = totalHeight();
		const Int blockX = textData->centered ? origin.x + size.x / 2 - blockWidth / 2 : origin.x + textData->leftMargin;
		const Int centeredY = std::max( origin.y, origin.y + size.y / 2 - blockHeight / 2 );
		Int y = textData->centeredVertically ? centeredY : origin.y + textData->topMargin;

		IRegion2D clip;
		clip.lo.x = origin.x;
		clip.lo.y = origin.y;
		clip.hi.x = origin.x + size.x;
		clip.hi.y = origin.y + size.y;

		const bool isWrapCentered = BitIsSet( status, WIN_STATUS_WRAP_CENTERED );
		for ( Line& line : m_lines )
		{
			const Int lineX = isWrapCentered ? blockX + ( blockWidth - line.width ) / 2 : blockX;
			drawLine( line, lineX, y, clip, textColor, dropColor );
			y += line.height;
		}
	}

	static void drawLine( Line& line, Int x, Int y, IRegion2D& clip, Color textColor, Color dropColor )
	{
		for ( Segment& segment : line.segments )
		{
			x += segment.gapBefore;
			const Int top = y + line.height - segment.font->height;
			const Color color = segment.style.hasColor ? segment.style.color : textColor;
			segment.string->setClipRegion( &clip );
			segment.string->draw( x, top, color, dropColor );
			x += segment.width;
		}
	}

	Int widestLine() const
	{
		Int widest = 0;
		for ( const Line& line : m_lines )
		{
			widest = std::max( widest, line.width );
		}
		return widest;
	}

	Int totalHeight() const
	{
		Int height = 0;
		for ( const Line& line : m_lines )
		{
			height += line.height;
		}
		return height;
	}

	void releaseStrings()
	{
		if ( TheDisplayStringManager == nullptr )
		{
			return;
		}

		for ( Line& line : m_lines )
		{
			for ( Segment& segment : line.segments )
			{
				if ( segment.string != nullptr )
				{
					TheDisplayStringManager->freeDisplayString( segment.string );
					segment.string = nullptr;
				}
			}
		}
	}

	std::vector<Piece> m_pieces;
	std::vector<Line> m_lines;
	GameFont* m_baseFont = nullptr;
	Int m_wrapWidth = -1;
	Int m_maxHeight = -1;
	Int m_pointSize = 0;
	DisplayString* m_measure = nullptr;
};

namespace
{
	struct BodyBinding
	{
		GameWinDrawFunc originalDraw;
		MacOSRichText* text;
	};

	std::map<GameWindow*, BodyBinding>& bodyBindings()
	{
		static std::map<GameWindow*, BodyBinding> bindings;
		return bindings;
	}

	void drawRichBody( GameWindow* window, WinInstanceData* instData )
	{
		const auto found = bodyBindings().find( window );
		if ( found == bodyBindings().end() )
		{
			return;
		}

		if ( found->second.originalDraw != nullptr )
		{
			found->second.originalDraw( window, instData );
		}
		found->second.text->draw( window );
	}
}

void MacOSRichMessageBox::show( const UnicodeString& title, const UnicodeString& markup )
{
	close();

	if ( TheWindowManager == nullptr || TheNameKeyGenerator == nullptr )
	{
		return;
	}

	m_box = MessageBoxOk( title, UnicodeString::TheEmptyString, nullptr );
	if ( m_box == nullptr )
	{
		return;
	}

	m_body = TheWindowManager->winGetWindowFromId( m_box, TheNameKeyGenerator->nameToKey( kMessageBodyName ) );
	if ( m_body == nullptr )
	{
		return;
	}

	m_text = new MacOSRichText( markup );
	bodyBindings()[m_body] = BodyBinding{ m_body->winGetDrawFunc(), m_text };
	m_body->winSetDrawFunc( drawRichBody );
}

void MacOSRichMessageBox::close()
{
	if ( isOpen() )
	{
		TheWindowManager->winDestroy( m_box );
	}

	releaseText();
	m_box = nullptr;
	m_body = nullptr;
}

bool MacOSRichMessageBox::isOpen() const
{
	if ( m_box == nullptr || TheWindowManager == nullptr )
	{
		return false;
	}

	for ( GameWindow* window = TheWindowManager->winGetWindowList(); window != nullptr; window = window->winGetNext() )
	{
		if ( window == m_box )
		{
			return true;
		}
	}
	return false;
}

void MacOSRichMessageBox::releaseText()
{
	if ( m_body != nullptr )
	{
		bodyBindings().erase( m_body );
	}

	delete m_text;
	m_text = nullptr;
}
