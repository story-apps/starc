#include "quotes_generator.h"

#include <QApplication>
#include <QRandomGenerator>
#include <QVector>


QuotesGenerator::Quote QuotesGenerator::generate(int _index)
{
    // clang-format off
    int index = 0;
    const QVector<Quote> quotes = {
        { index++,
          //: Cite of the Alfonso Cuarón
          QCoreApplication::translate("QuotesGenerator", "I learned there’s an amazing unexplored territory in terms of narrative. Before, I thought the unexplored territory was the form, the way you shoot a movie. Now, I’m learning about the beautiful marriage between form and narrative."),
          QCoreApplication::translate("QuotesGenerator", "Alfonso Cuarón") },
        { index++,
          //: Cite of the Alfonso Cuarón
          QCoreApplication::translate("QuotesGenerator", "When you’re doing a film, narrative is your most important tool, but it’s a tool to create a cinematographic experience, to create those moments that are beyond narrative, that are almost an abstraction of that moment that hits your psyche."),
          QCoreApplication::translate("QuotesGenerator", "Alfonso Cuarón") },
        { index++,
          //: Cite of the Alfred Hitchcock
          QCoreApplication::translate("QuotesGenerator", "To make a great film you need three things: the script, the script, and the script."),
          QCoreApplication::translate("QuotesGenerator", "Alfred Hitchcock") },
        { index++,
          //: Cite of the Alice Wu
          QCoreApplication::translate("QuotesGenerator", "I wrote a check for $1,000 to the NRA, gave it to my friend, and I said, I’m giving myself 5 weeks. On August 8, if this is not a fully written script and two people read it and confirm, you are sending that check in. And then I told all my friends. It was the most stressful five weeks of my life. It was like having a thousand agents breathing down my neck."),
          QCoreApplication::translate("QuotesGenerator", "Alice Wu") },
        { index++,
          //: Cite of the Allegra Goodman
          QCoreApplication::translate("QuotesGenerator", "Know your literary tradition, savor it, steal from it, but when you sit down to write, forget about worshiping greatness and fetishizing masterpieces."),
          QCoreApplication::translate("QuotesGenerator", "Allegra Goodman") },
        { index++,
          //: Cite of the Allen Ginsberg
          QCoreApplication::translate("QuotesGenerator", "To gain your own voice, you have to forget about having it heard."),
          QCoreApplication::translate("QuotesGenerator", "Allen Ginsberg") },
        { index++,
          //: Cite of the Andre Dubus III
          QCoreApplication::translate("QuotesGenerator", "I think the deeper you go into questions, the deeper or more interesting the questions get. And I think that’s the job of art."),
          QCoreApplication::translate("QuotesGenerator", "Andre Dubus III") },
        { index++,
          //: Cite of the Andre Gide
          QCoreApplication::translate("QuotesGenerator", "The most beautiful things are those that madness prompts and reason writes."),
          QCoreApplication::translate("QuotesGenerator", "Andre Gide") },
        { index++,
          //: Cite of the Annie Dillard
          QCoreApplication::translate("QuotesGenerator", "Do not hoard what seems good for a later place in the book, or for another book; give it, give it all, give it now."),
          QCoreApplication::translate("QuotesGenerator", "Annie Dillard") },
        { index++,
          //: Cite of the Ben Hecht
          QCoreApplication::translate("QuotesGenerator", "Out of the thousand writers huffing and puffing through movieland there are scarcely fifty men and women of wit or talent. The rest of the fraternity is deadwood. Yet, in a curious way, there is not much difference between the product of a good writer and a bad one. They both have to toe the same mark."),
          QCoreApplication::translate("QuotesGenerator", "Ben Hecht") },
        { index++,
          //: Cite of the Billy Wilder
          QCoreApplication::translate("QuotesGenerator", "If you have a problem with the third act, the real problem is in the first act."),
          QCoreApplication::translate("QuotesGenerator", "Billy Wilder") },
        { index++,
          //: Cite of the Bong Joon-Ho
          QCoreApplication::translate("QuotesGenerator", "I have a complex feeling about genre. I love it, but I hate it at the same time. I have the urge to make audiences thrill with the excitement of a genre, but I also try to betray and destroy the expectations placed on that genre."),
          QCoreApplication::translate("QuotesGenerator", "Bong Joon-Ho") },
        { index++,
          //: Cite of the Bong Joon-Ho
          QCoreApplication::translate("QuotesGenerator", "The multilevel, the conscious and the unconscious, is natural when I write scripts… when I come up with ideas and stories."),
          QCoreApplication::translate("QuotesGenerator", "Bong Joon-Ho") },
        { index++,
          //: Cite of the Carl Sandburg
          QCoreApplication::translate("QuotesGenerator", "Beware of advice — even this."),
          QCoreApplication::translate("QuotesGenerator", "Carl Sandburg") },
        { index++,
          //: Cite of the Charise Castro Smith
          QCoreApplication::translate("QuotesGenerator", "It’s so important just because what we see, we kind of validate; what we see, we can empathize with; what we see, we can sort of put ourselves into the shoes of. I think what I do as a storyteller and what we all do as storytellers is kind of fundamental to the fabric of society."),
          QCoreApplication::translate("QuotesGenerator", "Charise Castro Smith") },
        { index++,
          //: Cite of the Charlie Kaufman
          QCoreApplication::translate("QuotesGenerator", "A screenplay is an exploration. It’s about the thing you don’t know. It’s a step into the abyss. It necessarily starts somewhere, anywhere; there is a starting point, but the rest is undetermined."),
          QCoreApplication::translate("QuotesGenerator", "Charlie Kaufman") },
        { index++,
          //: Cite of the David Mamet
          QCoreApplication::translate("QuotesGenerator", "A good film script should be able to do completely without dialogue."),
          QCoreApplication::translate("QuotesGenerator", "David Mamet") },
        { index++,
          //: Cite of the Dee Rees
          QCoreApplication::translate("QuotesGenerator", "I care about subtext. If you get that right, the text will flow."),
          QCoreApplication::translate("QuotesGenerator", "Dee Rees") },
        { index++,
          //: Cite of the Dee Rees
          QCoreApplication::translate("QuotesGenerator", "Writing is really freeing because it’s the only part of the process where it’s just you and the characters and you are by yourself in a room and you can just hash it out. There are no limitations."),
          QCoreApplication::translate("QuotesGenerator", "Dee Rees") },
        { index++,
          //: Cite of the Edward Zwick
          QCoreApplication::translate("QuotesGenerator", "There is no reason why challenging themes and engaging stories have to be mutually exclusive - in fact, each can fuel the other. As a filmmaker, I want to entertain people first and foremost. If out of that comes a greater awareness and understanding of a time or a circumstance, then the hope is that change can happen."),
          QCoreApplication::translate("QuotesGenerator", "Edward Zwick") },
        { index++,
          //: Cite of the Elaine Loh
          QCoreApplication::translate("QuotesGenerator", "A lot of my ideas were crap, but bad ideas help get the ball rolling too. And bad ideas lead to better ideas."),
          QCoreApplication::translate("QuotesGenerator", "Elaine Loh") },
        { index++,
          //: Cite of the Elmore Leonard
          QCoreApplication::translate("QuotesGenerator", "If it sounds like writing, I rewrite it."),
          QCoreApplication::translate("QuotesGenerator", "Elmore Leonard") },
        { index++,
          //: Cite of the Elmore Leonard
          QCoreApplication::translate("QuotesGenerator", "If it sounds like writing, I rewrite it. Or, if proper usage gets in the way, it may have to go. I can’t allow what we learned in English composition to disrupt the sound and rhythm of the narrative."),
          QCoreApplication::translate("QuotesGenerator", "Elmore Leonard") },
        { index++,
          //: Cite of the Enid Bagnold
          QCoreApplication::translate("QuotesGenerator", "Who wants to become a writer? And why? Because it’s the answer to everything. … It’s the streaming reason for living. To note, to pin down, to build up, to create, to be astonished at nothing, to cherish the oddities, to let nothing go down the drain, to make something, to make a great flower out of life, even if it’s a cactus."),
          QCoreApplication::translate("QuotesGenerator", "Enid Bagnold") },
        { index++,
          //: Cite of the Ernest Hemingway
          QCoreApplication::translate("QuotesGenerator", "Critics are men who watch a battle from a high place then come down and shoot the survivors."),
          QCoreApplication::translate("QuotesGenerator", "Ernest Hemingway") },
        { index++,
          //: Cite of the Ernest Hemingway
          QCoreApplication::translate("QuotesGenerator", "It’s none of their business that you have to learn to write. Let them think you were born that way."),
          QCoreApplication::translate("QuotesGenerator", "Ernest Hemingway") },
        { index++,
          //: Cite of the Ernest Hemingway
          QCoreApplication::translate("QuotesGenerator", "Life breaks us all but in the end we are stronger in the broken places."),
          QCoreApplication::translate("QuotesGenerator", "Ernest Hemingway") },
        { index++,
          //: Cite of the Ernest Hemingway
          QCoreApplication::translate("QuotesGenerator", "The best people possess a feeling for beauty, the courage to take risks, the discipline to tell the truth, the capacity for sacrifice. Ironically, their virtues make them vulnerable; they are often wounded, sometimes destroyed."),
          QCoreApplication::translate("QuotesGenerator", "Ernest Hemingway") },
        { index++,
          //: Cite of the Ernest Hemingway
          QCoreApplication::translate("QuotesGenerator", "The first draft of anything is shit."),
          QCoreApplication::translate("QuotesGenerator", "Ernest Hemingway") },
        { index++,
          //: Cite of the Ernest Hemingway
          QCoreApplication::translate("QuotesGenerator", "We are all apprentices in a craft where no one ever becomes a master."),
          QCoreApplication::translate("QuotesGenerator", "Ernest Hemingway") },
        { index++,
          //: Cite of the Ernest Hemingway
          QCoreApplication::translate("QuotesGenerator", "When writing a novel a writer should create living people; people, not characters. A character is a caricature."),
          QCoreApplication::translate("QuotesGenerator", "Ernest Hemingway") },
        { index++,
          //: Cite of the Ernest Hemingway
          QCoreApplication::translate("QuotesGenerator", "You must be prepared to work always without applause."),
          QCoreApplication::translate("QuotesGenerator", "Ernest Hemingway") },
        { index++,
          //: Cite of the Ethan Coen
          QCoreApplication::translate("QuotesGenerator", "We wouldn’t have done it if we didn’t think we could have fun with it."),
          QCoreApplication::translate("QuotesGenerator", "Ethan Coen") },
        { index++,
          //: Cite of the Fran Walsh
          QCoreApplication::translate("QuotesGenerator", "The last thing I want to see at the movies is a version of my reality. I don’t want to see art imitating life."),
          QCoreApplication::translate("QuotesGenerator", "Fran Walsh") },
        { index++,
          //: Cite of the Francis Ford Coppola
          QCoreApplication::translate("QuotesGenerator", "I became quite successful very young, and it was mainly because I was so enthusiastic and I just worked so hard at it."),
          QCoreApplication::translate("QuotesGenerator", "Francis Ford Coppola") },
        { index++,
          //: Cite of the Fred Astaire
          QCoreApplication::translate("QuotesGenerator", " I inherited the knowledge that sometimes (talents) not only skip a generation, but sometimes run screaming from it. Ironically, it was the singing non-ability that helped feed my love of laughter."),
          QCoreApplication::translate("QuotesGenerator", "Fred Astaire") },
        { index++,
          //: Cite of the Fred Astaire
          QCoreApplication::translate("QuotesGenerator", "Either the camera will dance, or I will."),
          QCoreApplication::translate("QuotesGenerator", "Fred Astaire") },
        { index++,
          //: Cite of the Fred Zinnemann
          QCoreApplication::translate("QuotesGenerator", "Dialogue is a necessary evil."),
          QCoreApplication::translate("QuotesGenerator", "Fred Zinnemann") },
        { index++,
          //: Cite of the George Orwell
          QCoreApplication::translate("QuotesGenerator", "When I sit down to write a book, I do not say to myself, ‘I am going to produce a work of art.’ I write it because there is some lie that I want to expose, some fact to which I want to draw attention, and my initial concern is to get a hearing."),
          QCoreApplication::translate("QuotesGenerator", "George Orwell") },
        { index++,
          //: Cite of the George Singleton
          QCoreApplication::translate("QuotesGenerator", "Keep a small can of WD-40 on your desk—away from any open flames—to remind yourself that if you don’t write daily, you will get rusty."),
          QCoreApplication::translate("QuotesGenerator", "George Singleton") },
        { index++,
          //: Cite of the George Singleton
          QCoreApplication::translate("QuotesGenerator", "You do not have to explain every single drop of water contained in a rain barrel. You have to explain one drop—H2O. The reader will get it."),
          QCoreApplication::translate("QuotesGenerator", "George Singleton") },
        { index++,
          //: Cite of the Gore Vidal
          QCoreApplication::translate("QuotesGenerator", "Each writer is born with a repertory company in his head. Shakespeare has perhaps 20 players. … I have 10 or so, and that’s a lot. As you get older, you become more skillful at casting them."),
          QCoreApplication::translate("QuotesGenerator", "Gore Vidal") },
        { index++,
          //: Cite of the Greta Gerwig
          QCoreApplication::translate("QuotesGenerator", "Let your characters talk to each other and do things. Spend time with them — they’ll tell you who they are and what they’re up to."),
          QCoreApplication::translate("QuotesGenerator", "Greta Gerwig") },
        { index++,
          //: Cite of the Guillermo del Toro
          QCoreApplication::translate("QuotesGenerator", "To me, art and storytelling serve primal, spiritual functions in my daily life. Whether I’m telling a bedtime story to my kids or trying to mount a movie or write a short story or a novel, I take it very seriously."),
          QCoreApplication::translate("QuotesGenerator", "Guillermo del Toro") },
        { index++,
          //: Cite of the H. L. Mencken
          QCoreApplication::translate("QuotesGenerator", "There are no dull subjects, only dull writers"),
          QCoreApplication::translate("QuotesGenerator", "H. L. Mencken") },
        { index++,
          //: Cite of the Harper Lee
          QCoreApplication::translate("QuotesGenerator", "Characters make their own plot. The dimensions of the characters determine the action of the novel."),
          QCoreApplication::translate("QuotesGenerator", "Harper Lee") },
        { index++,
          //: Cite of the Harper Lee
          QCoreApplication::translate("QuotesGenerator", "I would advise anyone who aspires to a writing career that before developing his talent he would be wise to develop a thick hide."),
          QCoreApplication::translate("QuotesGenerator", "Harper Lee") },
        { index++,
          //: Cite of the Harper Lee
          QCoreApplication::translate("QuotesGenerator", "It’s not necessary to tell all you know."),
          QCoreApplication::translate("QuotesGenerator", "Harper Lee") },
        { index++,
          //: Cite of the Harper Lee
          QCoreApplication::translate("QuotesGenerator", "You never really understand a person until you consider things from his point of view."),
          QCoreApplication::translate("QuotesGenerator", "Harper Lee") },
        { index++,
          //: Cite of the Hayao Miyazaki
          QCoreApplication::translate("QuotesGenerator", "The creation of a single world comes from a huge number of fragments and chaos."),
          QCoreApplication::translate("QuotesGenerator", "Hayao Miyazaki") },
        { index++,
          //: Cite of the Henry David Thoreau
          QCoreApplication::translate("QuotesGenerator", "Write while the heat is in you. … The writer who postpones the recording of his thoughts uses an iron which has cooled to burn a hole with."),
          QCoreApplication::translate("QuotesGenerator", "Henry David Thoreau") },
        { index++,
          //: Cite of the Hunter S. Thompson
          QCoreApplication::translate("QuotesGenerator", "Not a wasted word. This has been a main point to my literary thinking all my life."),
          QCoreApplication::translate("QuotesGenerator", "Hunter S. Thompson") },
        { index++,
          //: Cite of the Jack Kerouac
          QCoreApplication::translate("QuotesGenerator", "Genius gives birth, talent delivers. What Rembrandt or Van Gogh saw in the night can never be seen again. Born writers of the future are amazed already at what they’re seeing now, what we’ll all see in time for the first time, and then see imitated many times by made writers."),
          QCoreApplication::translate("QuotesGenerator", "Jack Kerouac") },
        { index++,
          //: Cite of the Jack Kerouac
          QCoreApplication::translate("QuotesGenerator", "It ain’t whatcha write, it’s the way atcha write it."),
          QCoreApplication::translate("QuotesGenerator", "Jack Kerouac") },
        { index++,
          //: Cite of the Jean-Luc Godard
          QCoreApplication::translate("QuotesGenerator", "A story should have a beginning, a middle, and an end, but not necessarily in that order."),
          QCoreApplication::translate("QuotesGenerator", "Jean-Luc Godard") },
        { index++,
          //: Cite of the Jim Thompson
          QCoreApplication::translate("QuotesGenerator", "There is only one plot—things are not what they seem."),
          QCoreApplication::translate("QuotesGenerator", "Jim Thompson") },
        { index++,
          //: Cite of the Jim Tully, WD
          QCoreApplication::translate("QuotesGenerator", "Long patience and application saturated with your heart’s blood—you will either write or you will not—and the only way to find out whether you will or not is to try."),
          QCoreApplication::translate("QuotesGenerator", "Jim Tully, WD") },
        { index++,
          //: Cite of the Joel Coen
          QCoreApplication::translate("QuotesGenerator", "The characters are the result of two things — first, we elaborate them into fairly well-defined people through their dialogue, then they happen all over again, when the actor interprets them."),
          QCoreApplication::translate("QuotesGenerator", "Joel Coen") },
        { index++,
          //: Cite of the John August
          QCoreApplication::translate("QuotesGenerator", "I often say that a lot of my job, as a screenwriter, is sort of being a stock picker, in that I need to figure out what movies are probably going to get made because otherwise, I’m spending a lot of my time on something that’s trapped in 12-point courier."),
          QCoreApplication::translate("QuotesGenerator", "John August") },
        { index++,
          //: Cite of the John August
          QCoreApplication::translate("QuotesGenerator", "I think the greatest barrier I’ve always faced as a writer is just starting. Like all of us, I can just procrastinate and find some excuse for not actually getting started."),
          QCoreApplication::translate("QuotesGenerator", "John August") },
        { index++,
          //: Cite of the John Carpenter
          QCoreApplication::translate("QuotesGenerator", "In England, I am a horror movie director. In Germany, I am a filmmaker. In the US, I am a bum."),
          QCoreApplication::translate("QuotesGenerator", "John Carpenter") },
        { index++,
          //: Cite of the John Guare
          QCoreApplication::translate("QuotesGenerator", "Feydeau’s one rule of playwriting: Character A: My life is perfect as long as I don’t see Character B. Knock Knock. Enter Character B."),
          QCoreApplication::translate("QuotesGenerator", "John Guare") },
        { index++,
          //: Cite of the John Guare
          QCoreApplication::translate("QuotesGenerator", "James Joyce wrote the definitive work about Dublin while he was living in Switzerland. We're all where we come from. We all have our roots."),
          QCoreApplication::translate("QuotesGenerator", "John Guare") },
        { index++,
          //: Cite of the John Guare
          QCoreApplication::translate("QuotesGenerator", "You can read ten books and finally come across one detail, and it's like, \"now everything else makes sense. Now I know where I am.\""),
          QCoreApplication::translate("QuotesGenerator", "John Guare") },
        { index++,
          //: Cite of the Jordan Peele
          QCoreApplication::translate("QuotesGenerator", "I’ve noticed that the truth works… If you’re being yourself and you’re just using your own emotions, they can feel it. If you’re doing fake, they can feel it. It took me a while in comedy to realize that your truth is more powerful than your mask."),
          QCoreApplication::translate("QuotesGenerator", "Jordan Peele") },
        { index++,
          //: Cite of the Jordan Peele
          QCoreApplication::translate("QuotesGenerator", "That’s my advice with dealing with writer’s block. Follow the fun. If you aren’t having fun, you are doing it wrong."),
          QCoreApplication::translate("QuotesGenerator", "Jordan Peele") },
        { index++,
          //: Cite of the Joss Whedon
          QCoreApplication::translate("QuotesGenerator", "Always be yourself…unless you suck"),
          QCoreApplication::translate("QuotesGenerator", "Joss Whedon") },
        { index++,
          //: Cite of the Joyce Carol Oates
          QCoreApplication::translate("QuotesGenerator", "The first sentence can’t be written until the final sentence is written."),
          QCoreApplication::translate("QuotesGenerator", "Joyce Carol Oates") },
        { index++,
          //: Cite of the Jules Renard
          QCoreApplication::translate("QuotesGenerator", "Style is to forget all styles."),
          QCoreApplication::translate("QuotesGenerator", "Jules Renard") },
        { index++,
          //: Cite of the Larry L. King
          QCoreApplication::translate("QuotesGenerator", "Write. Rewrite. When not writing or rewriting, read. I know of no shortcuts."),
          QCoreApplication::translate("QuotesGenerator", "Larry L. King") },
        { index++,
          //: Cite of the Lawrence Block
          QCoreApplication::translate("QuotesGenerator", "One thing that helps is to give myself permission to write badly. I tell myself that I’m going to do my five or 10 pages no matter what, and that I can always tear them up the following morning if I want. I’ll have lost nothing—writing and tearing up five pages would leave me no further behind than if I took the day off."),
          QCoreApplication::translate("QuotesGenerator", "Lawrence Block") },
        { index++,
          //: Cite of the Leigh Brackett
          QCoreApplication::translate("QuotesGenerator", "Plot is people. Human emotions and desires founded on the realities of life, working at cross purposes, getting hotter and fiercer as they strike against each other until finally there’s an explosion—that’s Plot."),
          QCoreApplication::translate("QuotesGenerator", "Leigh Brackett") },
        { index++,
          //: Cite of the Lena Waithe
          QCoreApplication::translate("QuotesGenerator", "Your first obligation as a writer is to tell the truth and to tell a good story."),
          QCoreApplication::translate("QuotesGenerator", "Lena Waithe") },
        { index++,
          //: Cite of the Leslie Dixon
          QCoreApplication::translate("QuotesGenerator", "One rule of screenwriting: does the reader want to turn the page?"),
          QCoreApplication::translate("QuotesGenerator", "Leslie Dixon") },
        { index++,
          //: Cite of the Leslie Gordon Barnard
          QCoreApplication::translate("QuotesGenerator", "Don’t expect the puppets of your mind to become the people of your story. If they are not realities in your own mind, there is no mysterious alchemy in ink and paper that will turn wooden figures into flesh and blood."),
          QCoreApplication::translate("QuotesGenerator", "Leslie Gordon Barnard") },
        { index++,
          //: Cite of the Margaret Laurence
          QCoreApplication::translate("QuotesGenerator", "Animals are less alone with roaring than we are with all these words."),
          QCoreApplication::translate("QuotesGenerator", "Margaret Laurence") },
        { index++,
          //: Cite of the Margaret Laurence
          QCoreApplication::translate("QuotesGenerator", "Follow your heart, and you perish."),
          QCoreApplication::translate("QuotesGenerator", "Margaret Laurence") },
        { index++,
          //: Cite of the Margaret Laurence
          QCoreApplication::translate("QuotesGenerator", "I've never been able to force a novel. I always had the sense something being given to me. You can't sit around and wait until inspiration strikes, but neither can you force into being something that isn't there."),
          QCoreApplication::translate("QuotesGenerator", "Margaret Laurence") },
        { index++,
          //: Cite of the Margaret Laurence
          QCoreApplication::translate("QuotesGenerator", "If I hadn't had my children, I wouldn't have written more and better, I would have written less and worse."),
          QCoreApplication::translate("QuotesGenerator", "Margaret Laurence") },
        { index++,
          //: Cite of the Margaret Laurence
          QCoreApplication::translate("QuotesGenerator", "The struggle is not lost. I believe we have to live, as long as we live, in the expectation and hope of changing the world for the better. That may sound naive. It may even sound sentimental. Never mind: I believe it. What are we to live for, except life itself? And, with all our doubts, with all our flaws, with all our problems, I believe that we will carry on, with God's help."),
          QCoreApplication::translate("QuotesGenerator", "Margaret Laurence") },
        { index++,
          //: Cite of the Margaret Laurence
          QCoreApplication::translate("QuotesGenerator", "What goes on inside isn't ever the same as what goes on outside."),
          QCoreApplication::translate("QuotesGenerator", "Margaret Laurence") },
        { index++,
          //: Cite of the Margaret Laurence
          QCoreApplication::translate("QuotesGenerator", "When I say work I only mean writing. Everything else is just odd jobs."),
          QCoreApplication::translate("QuotesGenerator", "Margaret Laurence") },
        { index++,
          //: Cite of the Mark Twain
          QCoreApplication::translate("QuotesGenerator", "Courage is resistance to fear, mastery of fear – not absence of fear."),
          QCoreApplication::translate("QuotesGenerator", "Mark Twain") },
        { index++,
          //: Cite of the Mark Twain
          QCoreApplication::translate("QuotesGenerator", "First get your facts, then distort them at your leisure"),
          QCoreApplication::translate("QuotesGenerator", "Mark Twain") },
        { index++,
          //: Cite of the Mark Twain
          QCoreApplication::translate("QuotesGenerator", "Get your facts first, then you can distort them as you please."),
          QCoreApplication::translate("QuotesGenerator", "Mark Twain") },
        { index++,
          //: Cite of the Mark Twain
          QCoreApplication::translate("QuotesGenerator", "I have never let my schooling interfere with my education."),
          QCoreApplication::translate("QuotesGenerator", "Mark Twain") },
        { index++,
          //: Cite of the Mark Twain
          QCoreApplication::translate("QuotesGenerator", "If you tell the truth, you don’t have to remember anything."),
          QCoreApplication::translate("QuotesGenerator", "Mark Twain") },
        { index++,
          //: Cite of the Mark Twain
          QCoreApplication::translate("QuotesGenerator", "The secret of getting ahead is getting started."),
          QCoreApplication::translate("QuotesGenerator", "Mark Twain") },
        { index++,
          //: Cite of the Mark Twain
          QCoreApplication::translate("QuotesGenerator", "To succeed in life, you need two things: ignorance and confidence."),
          QCoreApplication::translate("QuotesGenerator", "Mark Twain") },
        { index++,
          //: Cite of the Maya Angelou
          QCoreApplication::translate("QuotesGenerator", "There is no greater agony than bearing an untold story inside you."),
          QCoreApplication::translate("QuotesGenerator", "Maya Angelou") },
        { index++,
          //: Cite of the Michaela Coel
          QCoreApplication::translate("QuotesGenerator", "You have to be true to your instinctive way of writing. You have to find your identity."),
          QCoreApplication::translate("QuotesGenerator", "Michaela Coel") },
        { index++,
          //: Cite of the Mindy Kaling
          QCoreApplication::translate("QuotesGenerator", "I just have my characters say my controversial opinions and then hide behind them."),
          QCoreApplication::translate("QuotesGenerator", "Mindy Kaling") },
        { index++,
          //: Cite of the Nora Ephron
          QCoreApplication::translate("QuotesGenerator", "As a young journalist, I thought that stories were simply what happened. As a screenwriter, I realized that we create stories by imposing narrative on the events that happen around us."),
          QCoreApplication::translate("QuotesGenerator", "Nora Ephron") },
        { index++,
          //: Cite of the Nora Ephron
          QCoreApplication::translate("QuotesGenerator", "My mother wanted us to understand that the tragedies of your life one day have the potential to be comic stories the next."),
          QCoreApplication::translate("QuotesGenerator", "Nora Ephron") },
        { index++,
          //: Cite of the Orson Welles
          QCoreApplication::translate("QuotesGenerator", "Here's an outdated quote! -- \"A typewriter needs only paper; a camera uses film, requires subsidiary equipment by the truckload and Wellington several hundreds of technicians. That is always the central fact about the filmmakers opposed to any other artist: he can never afford his own tools.\""),
          QCoreApplication::translate("QuotesGenerator", "Orson Welles") },
        { index++,
          //: Cite of the Paul Schrader
          QCoreApplication::translate("QuotesGenerator", "I could be just a writer very easily. I am not a writer. I am a screenwriter, which is half a filmmaker. … But it is not an art form, because screenplays are not works of art. They are invitations to others to collaborate on a work of art."),
          QCoreApplication::translate("QuotesGenerator", "Paul Schrader") },
        { index++,
          //: Cite of the Peter Greenaway
          QCoreApplication::translate("QuotesGenerator", "We don’t need books to make films. It’s the last thing we want — it turns cinema into the bastard art of illustration."),
          QCoreApplication::translate("QuotesGenerator", "Peter Greenaway") },
        { index++,
          //: Cite of the Peter Handke
          QCoreApplication::translate("QuotesGenerator", "If a nation loses its storytellers, it loses its childhood."),
          QCoreApplication::translate("QuotesGenerator", "Peter Handke") },
        { index++,
          //: Cite of the Philip Roth
          QCoreApplication::translate("QuotesGenerator", "The road to hell is paved with works-in-progress."),
          QCoreApplication::translate("QuotesGenerator", "Philip Roth") },
        { index++,
          //: Cite of the Phoebe Waller-Bridge
          QCoreApplication::translate("QuotesGenerator", "[T]he element of surprise is the most important thing and what keeps me interested in writing. I can feel if I’ve written that predictable or boring line, and I will carry that around with me all day."),
          QCoreApplication::translate("QuotesGenerator", "Phoebe Waller-Bridge") },
        { index++,
          //: Cite of the Phoebe Waller-Bridge
          QCoreApplication::translate("QuotesGenerator", "Once you know what makes someone angry, you can tell a lot about that person."),
          QCoreApplication::translate("QuotesGenerator", "Phoebe Waller-Bridge") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "A writer should have this little voice inside of you saying, Tell the truth. Reveal a few secrets here."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "Everything I learned as an actor, I have basically applied to writing."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "I consider myself a student of cinema. It’s almost like I am going for my professorship in cinema, and the day I die is the day I graduate. It is a lifelong study."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "I don’t think there’s anything to be afraid of. Failure brings great rewards — in the life of an artist."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "I steal from every movie ever made."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "I’ve made a choice, so far, to go on this road alone. Because this is my time. This is my time to make movies."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "If you just love movies enough, you can make a good one."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "Movies are not about the weekend that they’re released, and in the grand scheme of things, that’s probably the most unimportant time of a film’s life."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "There are a lot of bad screenplays, so if you write a good screenplay people are going to respond to it."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "To me, movies and music go hand in hand. When I’m writing a script, one of the first things I do is find the music I’m going to play for the opening sequence."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "Trying to make a feature film yourself with no money is the best film school you can do."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the Quentin Tarantino
          QCoreApplication::translate("QuotesGenerator", "You know, my problem with most screenwriting is it is a blueprint. It’s like they’re afraid to write the damn thing. And I’m a writer. That’s what I do. I want it to be written. I want it to work on the page first and foremost. So when I’m writing the script, I’m not thinking about the viewer watching the movie. I’m thinking about the reader reading the script."),
          QCoreApplication::translate("QuotesGenerator", "Quentin Tarantino") },
        { index++,
          //: Cite of the R.L. Stine
          QCoreApplication::translate("QuotesGenerator", "People say, ‘What advice do you have for people who want to be writers?’ I say, they don’t really need advice, they know they want to be writers, and they’re gonna do it. Those people who know that they really want to do this and are cut out for it, they know it."),
          QCoreApplication::translate("QuotesGenerator", "R.L. Stine") },
        { index++,
          //: Cite of the Ray Bradbury, WD
          QCoreApplication::translate("QuotesGenerator", "I don’t believe in being serious about anything. I think life is too serious to be taken seriously."),
          QCoreApplication::translate("QuotesGenerator", "Ray Bradbury, WD") },
        { index++,
          //: Cite of the Ray Bradbury, WD
          QCoreApplication::translate("QuotesGenerator", "I don’t need an alarm clock. My ideas wake me."),
          QCoreApplication::translate("QuotesGenerator", "Ray Bradbury, WD") },
        { index++,
          //: Cite of the Ray Bradbury, WD
          QCoreApplication::translate("QuotesGenerator", "Just write every day of your life. Read intensely. Then see what happens. Most of my friends who are put on that diet have very pleasant careers."),
          QCoreApplication::translate("QuotesGenerator", "Ray Bradbury, WD") },
        { index++,
          //: Cite of the Ray Bradbury, WD
          QCoreApplication::translate("QuotesGenerator", "Let the world burn through you. Throw the prism light, white hot, on paper."),
          QCoreApplication::translate("QuotesGenerator", "Ray Bradbury, WD") },
        { index++,
          //: Cite of the Raymond Chandler
          QCoreApplication::translate("QuotesGenerator", "The wise screen writer is he who wears his second-best suit, artistically speaking, and doesn’t take things too much to heart. He should have a touch of cynicism, but only a touch. The complete cynic is as useless to Hollywood as he is to himself. He should do the best he can without straining at it. He should be scrupulously honest about his work, but he should not expect scrupulous honesty in return. He won’t get it. And when he has had enough, he should say goodbye with a smile, because for all he knows he may want to go back."),
          QCoreApplication::translate("QuotesGenerator", "Raymond Chandler") },
        { index++,
          //: Cite of the Richard Ben Cramer
          QCoreApplication::translate("QuotesGenerator", "I’m out there to clean the plate. Once they’ve read what I’ve written on a subject, I want them to think, ‘That’s it!’ I think the highest aspiration people in our trade can have is that once they’ve written a story, nobody will ever try it again."),
          QCoreApplication::translate("QuotesGenerator", "Richard Ben Cramer") },
        { index++,
          //: Cite of the Rita Mae Brown
          QCoreApplication::translate("QuotesGenerator", "You sell a screenplay like you sell a car. If someone drives it off a cliff, that’s it."),
          QCoreApplication::translate("QuotesGenerator", "Rita Mae Brown") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "... don’t care if a reader hates one of my stories, just as long as he finishes the book."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "A life is made up of a great number of small incidents, and a small number of great ones."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "A little magic can take you a long way."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "A little nonsense now and then is relished by the wisest men."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "A person who has good thoughts cannot ever be ugly. You can have a wonky nose and a crooked mouth and a double chin and stick-out teeth, but if you have good thoughts they will shine out of your face like sunbeams and you will always look lovely."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "Don't gobblefunk around with words."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "Grown ups are complicated creatures, full of quirks and secrets."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "I think probably kindness is my number one attribute in a human being. I'll put it before any of the things like courage or bravery or generosity or anything else."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "If you are going to get anywhere in life you have to read a lot of books."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "Kindness - that simple word. To be kind - it covers everything, to my mind. If you're kind that's it."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "Somewhere inside all of us is the power to change the world."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "Those who don't believe in magic will never find it."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Roald Dahl
          QCoreApplication::translate("QuotesGenerator", "We make realities out of our dreams and dreams out of our realities. We are the dreamers of the dream."),
          QCoreApplication::translate("QuotesGenerator", "Roald Dahl") },
        { index++,
          //: Cite of the Robert Altman
          QCoreApplication::translate("QuotesGenerator", "I don’t think screenplay writing is the same as writing — I mean, I think it’s blueprinting."),
          QCoreApplication::translate("QuotesGenerator", "Robert Altman") },
        { index++,
          //: Cite of the Robert Bresson
          QCoreApplication::translate("QuotesGenerator", "The most ordinary word, when put into place, suddenly acquires brilliance. That is the brilliance with which your images must shine."),
          QCoreApplication::translate("QuotesGenerator", "Robert Bresson") },
        { index++,
          //: Cite of the Robert McKee
          QCoreApplication::translate("QuotesGenerator", "If the story you’re telling, is the story you’re telling, you’re in deep shit."),
          QCoreApplication::translate("QuotesGenerator", "Robert McKee") },
        { index++,
          //: Cite of the Robert McKee
          QCoreApplication::translate("QuotesGenerator", "Writing is a marathon, not a sprint."),
          QCoreApplication::translate("QuotesGenerator", "Robert McKee") },
        { index++,
          //: Cite of the Robert Rodriguez
          QCoreApplication::translate("QuotesGenerator", "Do not be told something is impossible. There is always a way."),
          QCoreApplication::translate("QuotesGenerator", "Robert Rodriguez") },
        { index++,
          //: Cite of the Robert Wise
          QCoreApplication::translate("QuotesGenerator", "My three Ps: passion, patience, perseverance. You have to do this if you’ve got to be a filmmaker."),
          QCoreApplication::translate("QuotesGenerator", "Robert Wise") },
        { index++,
          //: Cite of the Robin Swicord
          QCoreApplication::translate("QuotesGenerator", "You don’t learn how to write a screenplay by just reading screenplays and watching movies. It’s about developing the kind of mind that sees and makes drama. You can do this in a kind of holistic way by reading history and theology and psychology, reading great fiction and poetry, and plays. You develop an eye for the structures of everything and look for the patterns that help you become a dramatist."),
          QCoreApplication::translate("QuotesGenerator", "Robin Swicord") },
        { index++,
          //: Cite of the Roger Corman
          QCoreApplication::translate("QuotesGenerator", "You can make a movie about anything, as long as it has a hook to hang the advertising on."),
          QCoreApplication::translate("QuotesGenerator", "Roger Corman") },
        { index++,
          //: Cite of the Roman Polanski
          QCoreApplication::translate("QuotesGenerator", "Cinema should make you forget you are sitting in a theater."),
          QCoreApplication::translate("QuotesGenerator", "Roman Polanski") },
        { index++,
          //: Cite of the Ryan Murphy
          QCoreApplication::translate("QuotesGenerator", "With all of my work, it always takes a while for people to get it. It goes from, ‘What the hell is that?’ to ‘Oh, I like that.’"),
          QCoreApplication::translate("QuotesGenerator", "Ryan Murphy") },
        { index++,
          //: Cite of the Samuel Johnson
          QCoreApplication::translate("QuotesGenerator", "The greatest part of a writer’s time is spent in reading, in order to write; a man will turn over half a library to make one book."),
          QCoreApplication::translate("QuotesGenerator", "Samuel Johnson") },
        { index++,
          //: Cite of the Sofia Coppola
          QCoreApplication::translate("QuotesGenerator", "It seems that the greatest difficulty is to find the end. Don’t try to find it, it’s there already."),
          QCoreApplication::translate("QuotesGenerator", "Sofia Coppola") },
        { index++,
          //: Cite of the Spike Lee
          QCoreApplication::translate("QuotesGenerator", "A lot of times you get credit for stuff in your movies you didn’t intend to be there."),
          QCoreApplication::translate("QuotesGenerator", "Spike Lee") },
        { index++,
          //: Cite of the Spike Lee
          QCoreApplication::translate("QuotesGenerator", "A spine to my films that’s become more evident to me is that many are about the choices that people make… You go this way, or that way, and either way, there’s going to be consequences."),
          QCoreApplication::translate("QuotesGenerator", "Spike Lee") },
        { index++,
          //: Cite of the Stanley Kubrick
          QCoreApplication::translate("QuotesGenerator", "If it can be written, or thought, it can be filmed."),
          QCoreApplication::translate("QuotesGenerator", "Stanley Kubrick") },
        { index++,
          //: Cite of the Stephen King
          QCoreApplication::translate("QuotesGenerator", "Making people believe the unbelievable is no trick"),
          QCoreApplication::translate("QuotesGenerator", "Stephen King") },
        { index++,
          //: Cite of the Stephen King
          QCoreApplication::translate("QuotesGenerator", "When your story is ready for rewrite, cut it to the bone. Get rid of every ounce of excess fat. This is going to hurt; revising a story down to the bare essentials is always a little like murdering children, but it must be done."),
          QCoreApplication::translate("QuotesGenerator", "Stephen King") },
        { index++,
          //: Cite of the Steve Almond
          QCoreApplication::translate("QuotesGenerator", "All readers come to fiction as willing accomplices to your lies. Such is the basic goodwill contract made the moment we pick up a work of fiction."),
          QCoreApplication::translate("QuotesGenerator", "Steve Almond") },
        { index++,
          //: Cite of the Syd Field
          QCoreApplication::translate("QuotesGenerator", "What a person does is what he is, not what he says."),
          QCoreApplication::translate("QuotesGenerator", "Syd Field") },
        { index++,
          //: Cite of the Syd Field
          QCoreApplication::translate("QuotesGenerator", "Writing a screenplay is like climbing a mountain. When you’re climbing, all you can see is the rock in front of you and the rock directly above you. You can’t see where you’ve come from or where you’re going."),
          QCoreApplication::translate("QuotesGenerator", "Syd Field") },
        { index++,
          //: Cite of the Taika Waititi
          QCoreApplication::translate("QuotesGenerator", "My world is not spectacle and explosion. It’s two people talking."),
          QCoreApplication::translate("QuotesGenerator", "Taika Waititi") },
        { index++,
          //: Cite of the Tom Clancy
          QCoreApplication::translate("QuotesGenerator", "I do not over-intellectualize the production process. I try to keep it simple: Tell the damned story."),
          QCoreApplication::translate("QuotesGenerator", "Tom Clancy") },
        { index++,
          //: Cite of the Virginia Woolf
          QCoreApplication::translate("QuotesGenerator", "Every secret of a writer’s soul, every experience of his life, every quality of his mind, is written large in his works."),
          QCoreApplication::translate("QuotesGenerator", "Virginia Woolf") },
        { index++,
          //: Cite of the Virginia Woolf
          QCoreApplication::translate("QuotesGenerator", "Literature is strewn with the wreckage of men who have minded beyond reason the opinions of others."),
          QCoreApplication::translate("QuotesGenerator", "Virginia Woolf") },
        { index++,
          //: Cite of the William Goldman
          QCoreApplication::translate("QuotesGenerator", "Screenplay writing is not an art form. It’s a skill; it’s carpentry; it’s structure. I don’t mean to knock it — it ain’t easy. But if it’s all you do, if you only write screenplays, it is ultimately denigrating to the soul. You may get lucky and get rich, but you sure won’t get happy."),
          QCoreApplication::translate("QuotesGenerator", "William Goldman") },
        { index++,
          //: Cite of the William S. Burroughs
          QCoreApplication::translate("QuotesGenerator", "Cheat your landlord if you can and must, but do not try to shortchange the Muse. It cannot be done. You can’t fake quality any more than you can fake a good meal."),
          QCoreApplication::translate("QuotesGenerator", "William S. Burroughs") }, };
    // clang-format on

    if (_index == -1) {
        _index = QRandomGenerator::global()->bounded(0, quotes.size());
    }
    return quotes[_index];
}
