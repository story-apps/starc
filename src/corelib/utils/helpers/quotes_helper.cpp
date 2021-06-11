#include "quotes_helper.h"

#include <QApplication>
#include <QRandomGenerator>
#include <QVector>


QuotesHelper::Quote QuotesHelper::generateQuote(int _index)
{
    int index = 0;
    const QVector<Quote> quotes
        //
        // https://screencraft.org/2013/01/23/50-great-screenwriting-quotes/
        //
        = { { index++, "Dialogue is a necessary evil.", "Fred Zinnemann" },
            { index++, "The road to hell is paved with works-in-progress.", "Philip Roth" },
            { index++, "There is only one plot—things are not what they seem.", "Jim Thompson" },
            { index++,
              "The most beautiful things are those that madness prompts and reason writes.",
              "Andre Gide" },
            { index++,
              "Literature is strewn with the wreckage of men who have minded beyond reason the "
              "opinions of others.",
              "Virginia Woolf" },
            { index++, "If it sounds like writing, I rewrite it.", "Elmore Leonard" },
            { index++, "Cinema should make you forget you are sitting in a theater.",
              "Roman Polanski" },
            { index++, "Do not be told something is impossible. There is always a way.",
              "Robert Rodriguez" },
            { index++,
              "My three Ps: passion, patience, perseverance. You have to do this if you’ve got to "
              "be a filmmaker.",
              "Robert Wise" },
            { index++, "If it can be written, or thought, it can be filmed.", "Stanley Kubrick" },
            { index++,
              "I became quite successful very young, and it was mainly because I was so "
              "enthusiastic and I just worked so hard at it.",
              "Francis Ford Coppola" },
            { index++, "I steal from every movie ever made.", "Quentin Tarantino" },
            { index++,
              "I don’t think screenplay writing is the same as writing — I mean, I think it’s "
              "blueprinting.",
              "Robert Altman" },
            { index++,
              "The most ordinary word, when put into place, suddenly acquires brilliance. That is "
              "the brilliance with which your images must shine.",
              "Robert Bresson" },
            { index++,
              "You sell a screenplay like you sell a car. If someone drives it off a cliff, that’s "
              "it.",
              "Rita Mae Brown" },
            { index++, "A good film script should be able to do completely without dialogue.",
              "David Mamet" },
            { index++, "To gain your own voice, you have to forget about having it heard.",
              "Allen Ginsberg" },
            { index++,
              "Cheat your landlord if you can and must, but do not try to shortchange the Muse. It "
              "cannot be done. You can’t fake quality any more than you can fake a good meal.",
              "William S. Burroughs" },
            { index++,
              "All readers come to fiction as willing accomplices to your lies. Such is the basic "
              "goodwill contract made the moment we pick up a work of fiction.",
              "Steve Almond" },
            { index++, "It ain’t whatcha write, it’s the way atcha write it.", "Jack Kerouac" },
            { index++,
              "Not a wasted word. This has been a main point to my literary thinking all my life.",
              "Hunter S. Thompson" },
            { index++,
              "... don’t care if a reader hates one of my stories, just as long as he finishes the "
              "book.",
              "Roald Dahl" },
            { index++, "We are all apprentices in a craft where no one ever becomes a master.",
              "Ernest Hemingway" },
            { index++,
              "Every secret of a writer’s soul, every experience of his life, every quality of his "
              "mind, is written large in his works.",
              "Virginia Woolf" },
            { index++, "If a nation loses its storytellers, it loses its childhood.",
              "Peter Handke" },
            { index++,
              "Write. Rewrite. When not writing or rewriting, read. I know of no shortcuts.",
              "Larry L. King" },
            { index++,
              "Know your literary tradition, savor it, steal from it, but when you sit down to "
              "write, forget about worshiping greatness and fetishizing masterpieces.",
              "Allegra Goodman" },
            { index++, "Style is to forget all styles.", "Jules Renard" },
            { index++,
              "I do not over-intellectualize the production process. I try to keep it simple: Tell "
              "the damned story.",
              "Tom Clancy" },
            { index++, "The first sentence can’t be written until the final sentence is written.",
              "Joyce Carol Oates" },
            { index++,
              "Long patience and application saturated with your heart’s blood — you will either "
              "write or you will not — and the only way to find out whether you will or not is to "
              "try.",
              "Jim Tully, WD" },
            { index++, "Beware of advice — even this.", "Carl Sandburg" },
            { index++,
              "I would advise anyone who aspires to a writing career that before developing his "
              "talent he would be wise to develop a thick hide.",
              "Harper Lee" },
            { index++,
              "I think the deeper you go into questions, the deeper or more interesting the "
              "questions get. And I think that’s the job of art.",
              "Andre Dubus III" },
            { index++, "I don’t need an alarm clock. My ideas wake me.", "Ray Bradbury, WD" },
            { index++,
              "Just write every day of your life. Read intensely. Then see what happens. Most of "
              "my friends who are put on that diet have very pleasant careers.",
              "Ray Bradbury, WD" },
            { index++,
              "Let the world burn through you. Throw the prism light, white hot, on paper.",
              "Ray Bradbury, WD" },
            { index++,
              "I don’t believe in being serious about anything. I think life is too serious to be "
              "taken seriously.",
              "Ray Bradbury, WD" },
            { index++,
              "It’s none of their business that you have to learn to write. Let them think you "
              "were born that way.",
              "Ernest Hemingway" },
            { index++, "When I say work I only mean writing. Everything else is just odd jobs.",
              "Margaret Laurence" },
            { index++,
              "Do not hoard what seems good for a later place in the book, or for another book; "
              "give it, give it all, give it now.",
              "Annie Dillard" },
            { index++,
              "When writing a novel a writer should create living people; people, not characters. "
              "A character is a caricature.",
              "Ernest Hemingway" },
            { index++,
              "Write while the heat is in you. … The writer who postpones the recording of his "
              "thoughts uses an iron which has cooled to burn a hole with.",
              "Henry David Thoreau" },
            { index++,
              "You can make a movie about anything, as long as it has a hook to hang the "
              "advertising on.",
              "Roger Corman" },
            { index++,
              "A lot of times you get credit for stuff in your movies you didn’t intend to be "
              "there.",
              "Spike Lee" },
            { index++, "Get your facts first, then you can distort them as you please.",
              "Mark Twain" },
            { index++,
              "Feydeau’s one rule of playwriting: Character A: My life is perfect as long as I "
              "don’t see Character B. Knock Knock. Enter Character B.",
              "John Guare" } };
    if (_index == -1) {
        _index = QRandomGenerator::global()->bounded(0, quotes.size());
    }
    return quotes[_index];
}
