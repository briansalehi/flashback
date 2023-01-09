/// \page interface_page Interface Design
/// \brief Design of Application Programming Interface
/// \tableofcontents
///
/// \section dashboard_space Dashboard
///
/// Dashboard is the main page of the program where user can choose their next step.\n
/// Either of these steps are one of the available spaces in the program explained
/// below.
///
/// In dashboard, there are a few sections where user can enter:
///
/// 1. \emoji :books: \ref library_space : A space where you can review the books and resources you already used.
/// 2. \emoji :running: \ref trainer_space : A space to practice subjects by answering questions and comparing your answers.
/// 3. \emoji :mortar_board: \ref tutorial_space : A space to watch video courses teaching you everything about a subject.
/// 4. \emoji :rocket: \ref roadmap_space : A space to find your way through the skills you need to aquire to become an expert.
///
/// \startuml
/// !theme cyborg
/// rectangle Library {
///     usecase "Resources" as res
///     usecase "Notes" as n
///     usecase "Reference" as ref
///
///     res <-- n
///     n <-- ref
/// }
///
/// rectangle Trainer {
///     usecase "Subjects" as s
///     usecase "Topics" as t
///     usecase "Practices" as p
///     usecase "References" as ref2
///
///     s <-- t
///     t <-- p
///     p <-- ref2
/// }
/// rectangle Tutorials {
/// }
/// rectangle Roadmap {
/// }
/// \enduml
///
/// In the program, some parameters will be used program to monitor user activity.\n
/// Of these parameters, two are time related and will be used to remember
/// how much time it took from user to actively interact with the program:
///
/// - \ref session_interface
/// - \ref progress_interface
///
/// \subsection session_interface Session
///
/// Session represents user activity, and the time spent in each session is recorded.\n
/// Each session time span is the sum of all the time spent for each
/// \ref practice_definition during that session.
///
/// If program halts during a session without completing at least one practice,
/// the whole session time recorded shall not be stored and be dropped instead.
///
/// Each session record shall be stored in a non-volatile memory in order to be used
/// for next sessions.
/// This stored data containing previous session durations
/// shall be called \ref progress_interface.
///
/// \startuml
/// abstract class Session {
///     - practices: std::vector<Practice>
///     - estimated_time: std::chrono::duration
///     - elapsed_time: std::chrono::duration
///     - epoch_time: std::chrono::duration
///     - current_practice: std::shared_ptr<Practice>
/// }
/// \enduml
///
/// \subsection progress_interface Progress
///
/// \startuml
/// abstract class progress {
/// }
/// \enduml
///
/// \section library_space Library
///
/// This section consists of two objects:
///
/// - \ref resource_interface
/// - \ref note_interface
///
/// In library section, User can search through resources and select one.\n
/// Then they will be able to see the notes captured from that resource.
///
/// Notes can be used to review a resource. Though, user should be aware that
/// following the subject of that resource only by using that resource is not a
/// wise choice, but it's best to use \ref tutorial_space to learn that subject
/// and then use \ref trainer_space space to practice the topics
/// of that subject.
///
/// Resources will be loaded into the program from any available database,
/// either files in the first stage of the program or relational or non-relational
/// databases, through \ref loaders.
///
/// \subsection resource_interface Resource
///
/// \ref book is an abstract class in program which holds information
/// of a specific real \ref book_entity .\n
/// A \ref book also holds series of \ref practice objects which is required for
/// recollection procedure.
///
/// A resource may be a link, a book, a course, or an article.\n
/// So resource must be an interface, so that its derivations extend it.
///
/// The resource interface should at least have following core methods:
///
/// \startuml
/// abstract class Resource {
///     # title(): std::string
///     # notes(): std::set<Note>
/// }
///
/// class Book {
///     # chapters(): unsigned int
/// }
///
/// class Course {
///     # duration(): std::chrono::duration
/// }
///
/// Resource <|-- Book
/// Resource <|-- Course
/// \enduml
///
/// \subsection note_interface Note
///
/// \ref note_definition objects are simple data objects holding text
/// values picked directly from their corresponding \ref resource_definition objects.
///
/// \startuml
/// struct Note {
///     + title: std::string
///     + description: std::string
///     + position: Position
///     + collected: bool
///     # operator<(other: Note const&): bool
///     # operator!=(other: Note const&): bool
/// }
///
/// abstract class Position {
///     # operator<(other: Position const&): bool
///     # operator!=(other: Position const&): bool
/// 
/// }
///
/// Note <|-- Position
/// \enduml
///
/// Notes hold text from resources, either a resource be a video or a book, the final
/// conversion of them will be texual notes.
///
/// Notes have a flag indicating whether it has collected from \ref library_definition
/// to the \ref recollection_definition, because notes are the final product of
/// \ref library_definition, and they will be copied into the \ref recollection_definition
/// and will be modified accordingly so that they turn into question-answer form.
///
/// \subsection position_interface Position
///
/// Positions are resource specific locations where the text was taken from.\n
/// Positions might locate pages and chapters of a book, or a duration of course.
///
/// \startuml
/// abstract class Position {
///     # operator<(other: Position const&): bool
///     # operator!=(other: Position const&): bool
/// }
///
/// class BookPosition {
/// }
///
/// class CoursePosition {
/// }
///
/// Position <|-- BookPosition
/// Position <|-- CoursePosition
/// \enduml
///
/// \section tutorial_space Tutorial
///
///
///
/// \section trainer_space Trainer
///
/// Trainer space of the program consists of:
///
/// - \ref subject_interface
/// - \ref topic_interface
/// - \ref practice_interface
/// - \ref reference_interface
///
/// \subsection subject_interface Subject
///
/// Subject is the material in which \ref topic_interface will be held.
///
/// \startuml
/// abstract class Subject {
///     # add_topic(topic: Topic): bool
///     # topics(): std::vector<Topic>
///     # queue_practices(): std::queue<Practice>
/// }
/// \enduml
///
/// \subsection topic_interface Topic
///
/// Topics are domain specific entities which explain one and only one
/// material in a subject.
///
/// Each topic shall contain a few practices so that during each \ref session_definition,
/// only one practice can be picked and trained in order to have enough time
/// practicing all topics.
///
/// \startuml
/// abstract class Topic {
///     # add_practice(practice: Practice): bool
///     # practices(): std::queue<Practice>
/// }
/// \enduml
///
/// \subsection practice_interface Practice
///
/// A practice is an issue explaining a topic and is the main value of the
/// flashback program.
///
/// Each practice is a question which should be answered by user, and it
/// already has the answer to help user correct themselves in case of hesitation.
///
/// \startuml
/// abstract class Practice {
///     # question(): std::string
///     # answer(): std::string
///     # references(): std::vector<Reference>
///     # resources(): std::vector<Resource>
/// }
/// \enduml
///
/// \subsection reference_interface Reference
///
/// A reference is a pointer to some real entity in which the \ref practice_interface
/// relates to.\n
/// References are usually links to \ref resource_interface objects.
///
/// \startuml
/// abstract class Reference {
///     # target(): Resource
///     # position(): std::string
/// }
/// \enduml
///
///
/// \section command_line_interface Command Line Interface Design
///
/// Command line interface shall be designed as follows:
///
/// - no options given will result in printing help string
/// - starting to practice requires option `--start`.
/// - `--start` option initiates an interactive shell
///
///
/// \section program_flow Program Flow
///
/// Program runs in command line at its first stages, so options should be
/// carefully used to deal with the complexity of the program.\n
/// 
/// \see command_line_interface
///
/// Program starts by first loading the database.\n
/// This database is a repository at its first stages, and then it will be moved
/// into a proper database during development.
///
/// User will not be involved into the program execution process, and program
/// will sort all the user requirements automatically.\n
/// These requirements include:
///
/// - fetch all the practices from database
/// - sort all practices based on their topics
/// - pick up practices with shortest overall time span
/// - estimate the time it takes from user to solve all the practices
/// - evaluate the time that user takes to solve practices
/// - save user progress
/// - exit from program
///
/// This activity diagram is a simple flow of the program when user starts practicing.
///
/// \startuml
/// start
/// :Connect to database;
/// :Fetch practices;
/// :Create practice objects;
/// :Sort practices by topics;
/// :Estimate session time duration;
/// while (Topic list) is (next topic)
///     :Check time duration of practices;
///     :Pick a practice used long ago;
///     :Push practice in question queue;
/// endwhile (empty)
/// :Start session;
/// repeat :Ask question;
///     :Start practice timer;
///     if (User asks for answer) then
///         :Show the answer;
///     endif
///     :Stop practice timer;
///     :Record practice time duration;
///     if (User stops program) then
///         #pink:exit;
///         kill
///     endif
/// backward :Pop next practice from queue;
/// repeat while (User continues)
/// end
/// \enduml
///
/// \section roadmap_space Roadmap
///
///
///