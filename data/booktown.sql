--
-- Selected TOC Entries:
--
--
-- TOC Entry ID 1 (OID 0)
--
-- Name: booktown Type: DATABASE Owner: postgres
--

Create Database "booktown";

\connect booktown jharris

BEGIN;

--
-- TOC Entry ID 2 (OID 2991542)
--
-- Name: DATABASE "booktown" Type: COMMENT Owner: 
--

COMMENT ON DATABASE "booktown" IS 'The Book Town Database.';

--
-- TOC Entry ID 33 (OID 3629264)
--
-- Name: books Type: TABLE Owner: manager
--

CREATE TABLE "books" (
	"id" integer NOT NULL,
	"title" text NOT NULL,
	"author_id" integer,
	"subject_id" integer,
	Constraint "books_id_pkey" Primary Key ("id")
);

CREATE TABLE "publishers" (
	"id" integer NOT NULL,
	"name" text,
	"address" text,
	Constraint "publishers_pkey" Primary Key ("id")
);

CREATE TABLE "authors" (
	"id" integer NOT NULL,
	"last_name" text,
	"first_name" text,
	Constraint "authors_pkey" Primary Key ("id")
);

CREATE TABLE "numeric_values" (
	"num" numeric(30,6)
);

--
-- TOC Entry ID 20 (OID 3390866)
--
-- Name: daily_inventory Type: TABLE Owner: postgres
--

CREATE TABLE "daily_inventory" (
	"isbn" text,
	"is_stocked" boolean
);

--
-- TOC Entry ID 21 (OID 3391084)
--
-- Name: money_example Type: TABLE Owner: postgres
--

CREATE TABLE "money_example" (
	"money_cash" money,
	"numeric_cash" numeric(6,2)
);

--
-- TOC Entry ID 22 (OID 3391184)
--
-- Name: shipments Type: TABLE Owner: postgres
--

CREATE TABLE "shipments" (
	"id" integer DEFAULT nextval('"shipments_ship_id_seq"'::text) NOT NULL,
	"customer_id" integer,
	"isbn" text,
	"ship_date" timestamp with time zone
);

--
-- TOC Entry ID 24 (OID 3391454)
--
-- Name: customers Type: TABLE Owner: manager
--

CREATE TABLE "customers" (
	"id" integer NOT NULL,
	"last_name" text,
	"first_name" text,
	Constraint "customers_pkey" Primary Key ("id")
);

--
-- TOC Entry ID 15 (OID 3389632)
--
-- Name: states Type: TABLE Owner: postgres
--

CREATE TABLE "states" (
	"id" integer NOT NULL,
	"name" text,
	"abbreviation" character(2),
	Constraint "state_pkey" Primary Key ("id")
);

--
-- TOC Entry ID 16 (OID 3389702)
--
-- Name: my_list Type: TABLE Owner: postgres
--

CREATE TABLE "my_list" (
	"todos" text
);

--
-- TOC Entry ID 17 (OID 3390348)
--
-- Name: stock Type: TABLE Owner: postgres
--

CREATE TABLE "stock" (
	"isbn" text NOT NULL,
	"cost" numeric(5,2),
	"retail" numeric(5,2),
	"stock" integer,
	Constraint "stock_pkey" Primary Key ("isbn")
);

CREATE TABLE "book_queue" (
	"title" text NOT NULL,
	"author_id" integer,
	"subject_id" integer,
	"approved" boolean
);

CREATE TABLE "stock_backup" (
	"isbn" text,
	"cost" numeric(5,2),
	"retail" numeric(5,2),
	"stock" integer
);


CREATE TABLE "favorite_books" (
	"employee_id" integer,
	"books" text[]
);

CREATE TABLE "employees" (
	"id" integer NOT NULL,
	"last_name" text NOT NULL,
	"first_name" text,
	CONSTRAINT "employees_id" CHECK ((id > 100)),
	Constraint "employees_pkey" Primary Key ("id")
);

CREATE TABLE "editions" (
	"isbn" text NOT NULL,
	"book_id" integer,
	"edition" integer,
	"publisher_id" integer,
	"publication" date,
	"type" character(1),
	CONSTRAINT "integrity" CHECK (((book_id NOTNULL) AND (edition NOTNULL))),
	Constraint "pkey" Primary Key ("isbn")
);

CREATE TABLE "distinguished_authors" (
	"award" text
)
INHERITS ("authors");

CREATE TABLE "favorite_authors" (
	"employee_id" integer,
	"authors_and_titles" text[]
);

CREATE TABLE "text_sorting" (
	"letter" character(1)
);


CREATE TABLE "subjects" (
	"id" integer NOT NULL,
	"subject" text,
	"location" text,
	Constraint "subjects_pkey" Primary Key ("id")
);

CREATE TABLE "alternate_stock" (
	"isbn" text,
	"cost" numeric(5,2),
	"retail" numeric(5,2),
	"stock" integer
);

--
-- TOC Entry ID 40 (OID 3752020)
--
-- Name: book_backup Type: TABLE Owner: postgres
--

CREATE TABLE "book_backup" (
	"id" integer,
	"title" text,
	"author_id" integer,
	"subject_id" integer
);

CREATE TABLE "schedules" (
        "employee_id" integer NOT NULL,
        "schedule" text,
        Constraint "schedules_pkey" Primary Key ("employee_id")
);


COPY "publishers"  FROM stdin;
150	Kids Can Press	Kids Can Press, 29 Birch Ave. Toronto,ONM4V 1E2
91	Henry Holt & Company, Inc.	Henry Holt & Company, Inc. 115 West 18th Street New York, NY 10011
113	O'Reilly & Associates	O'Reilly & Associates, Inc. 101 Morris St, Sebastopol, CA 95472
62	Watson-Guptill Publications	1515 Boradway, New York, NY 10036
105	Noonday Press	Farrar Straus & Giroux Inc, 19 Union Square W, New York, NY 10003
99	Ace Books	The Berkley Publishing Group, Penguin Putnam Inc, 375 Hudson St, New York, NY 10014
101	Roc	Penguin Putnam Inc, 375 Hudson St, New York, NY 10014
163	Mojo Press	Mojo Press, PO Box 1215, Dripping Springs, TX 78720
171	Books of Wonder	Books of Wonder, 16 W. 18th St. New York, NY, 10011
102	Penguin	Penguin Putnam Inc, 375 Hudson St, New York, NY 10014
75	Doubleday	Random House, Inc, 1540 Broadway, New York, NY 10036
65	HarperCollins	HarperCollins Publishers, 10 E 53rd St, New York, NY 10022
59	Random House	Random House, Inc, 1540 Broadway, New York, NY 10036
\.
--
-- Data for TOC Entry ID 113 (OID 3389594)
--
-- Name: authors Type: TABLE DATA Owner: manager
--


COPY "authors"  FROM stdin;
1111	Denham	Ariel
1212	Worsley	John
15990	Bourgeois	Paulette
25041	Bianco	Margery Williams
16	Alcott	Louisa May
4156	King	Stephen
1866	Herbert	Frank
1644	Hogarth	Burne
2031	Brown	Margaret Wise
115	Poe	Edgar Allen
7805	Lutz	Mark
7806	Christiansen	Tom
1533	Brautigan	Richard
1717	Brite	Poppy Z.
2112	Gorey	Edward
2001	Clarke	Arthur C.
1213	Brookins	Andrew
\.
--
-- Data for TOC Entry ID 114 (OID 3389632)
--
-- Name: states Type: TABLE DATA Owner: postgres
--


COPY "states"  FROM stdin;
42	Washington	WA
51	Oregon	OR
\.
--
-- Data for TOC Entry ID 115 (OID 3389702)
--
-- Name: my_list Type: TABLE DATA Owner: postgres
--


COPY "my_list"  FROM stdin;
Pick up laundry.
Send out bills.
Wrap up Grand Unifying Theory for publication.
\.
--
-- Data for TOC Entry ID 116 (OID 3390348)
--
-- Name: stock Type: TABLE DATA Owner: postgres
--


COPY "stock"  FROM stdin;
0385121679	29.00	36.95	65
039480001X	30.00	32.95	31
0394900014	23.00	23.95	0
044100590X	36.00	45.95	89
0441172717	17.00	21.95	77
0451160916	24.00	28.95	22
0451198492	36.00	46.95	0
0451457994	17.00	22.95	0
0590445065	23.00	23.95	10
0679803335	20.00	24.95	18
0694003611	25.00	28.95	50
0760720002	18.00	23.95	28
0823015505	26.00	28.95	16
0929605942	19.00	21.95	25
1885418035	23.00	24.95	77
0394800753	16.00	16.95	4
\.
--
-- Data for TOC Entry ID 117 (OID 3390653)
--
-- Name: numeric_values Type: TABLE DATA Owner: postgres
--


COPY "numeric_values"  FROM stdin;
68719476736.000000
68719476737.000000
6871947673778.000000
999999999999999999999999.999900
999999999999999999999999.999999
-999999999999999999999999.999999
-100000000000000000000000.999999
1.999999
2.000000
2.000000
999999999999999999999999.999999
999999999999999999999999.000000
\.
--
-- Data for TOC Entry ID 118 (OID 3390866)
--
-- Name: daily_inventory Type: TABLE DATA Owner: postgres
--


COPY "daily_inventory"  FROM stdin;
039480001X	t
044100590X	t
0451198492	f
0394900014	f
0441172717	t
0451160916	f
0385121679	\N
\.
--
-- Data for TOC Entry ID 119 (OID 3391084)
--
-- Name: money_example Type: TABLE DATA Owner: postgres
--


COPY "money_example"  FROM stdin;
$12.24	12.24
\.
--
-- Data for TOC Entry ID 120 (OID 3391184)
--
-- Name: shipments Type: TABLE DATA Owner: postgres
--


COPY "shipments"  FROM stdin;
375	142	039480001X	2001-08-06 09:29:21-07
323	671	0451160916	2001-08-14 10:36:41-07
998	1045	0590445065	2001-08-12 12:09:47-07
749	172	0694003611	2001-08-11 10:52:34-07
662	655	0679803335	2001-08-09 07:30:07-07
806	1125	0760720002	2001-08-05 09:34:04-07
102	146	0394900014	2001-08-11 13:34:08-07
813	112	0385121679	2001-08-08 09:53:46-07
652	724	1885418035	2001-08-14 13:41:39-07
599	430	0929605942	2001-08-10 08:29:42-07
969	488	0441172717	2001-08-14 08:42:58-07
433	898	044100590X	2001-08-12 08:46:35-07
660	409	0451457994	2001-08-07 11:56:42-07
310	738	0451198492	2001-08-15 14:02:01-07
510	860	0823015505	2001-08-14 07:33:47-07
997	185	039480001X	2001-08-10 13:47:52-07
999	221	0451160916	2001-08-14 13:45:51-07
56	880	0590445065	2001-08-14 13:49:00-07
72	574	0694003611	2001-08-06 07:49:44-07
146	270	039480001X	2001-08-13 09:42:10-07
981	652	0451160916	2001-08-08 08:36:44-07
95	480	0590445065	2001-08-10 07:29:52-07
593	476	0694003611	2001-08-15 11:57:40-07
977	853	0679803335	2001-08-09 09:30:46-07
117	185	0760720002	2001-08-07 13:00:48-07
406	1123	0394900014	2001-08-13 09:47:04-07
340	1149	0385121679	2001-08-12 13:39:22-07
871	388	1885418035	2001-08-07 11:31:57-07
1000	221	039480001X	2001-09-14 16:46:32-07
1001	107	039480001X	2001-09-14 17:42:22-07
754	107	0394800753	2001-08-11 09:55:05-07
458	107	0394800753	2001-08-07 10:58:36-07
189	107	0394800753	2001-08-06 11:46:36-07
720	107	0394800753	2001-08-08 10:46:13-07
1002	107	0394800753	2001-09-22 11:23:28-07
2	107	0394800753	2001-09-22 20:58:56-07
\.
--
-- Data for TOC Entry ID 121 (OID 3391454)
--
-- Name: customers Type: TABLE DATA Owner: manager
--


COPY "customers"  FROM stdin;
107	Jackson	Annie
112	Gould	Ed
142	Allen	Chad
146	Williams	James
172	Brown	Richard
185	Morrill	Eric
221	King	Jenny
270	Bollman	Julie
388	Morrill	Royce
409	Holloway	Christine
430	Black	Jean
476	Clark	James
480	Thomas	Rich
488	Young	Trevor
574	Bennett	Laura
652	Anderson	Jonathan
655	Olson	Dave
671	Brown	Chuck
723	Eisele	Don
724	Holloway	Adam
738	Gould	Shirley
830	Robertson	Royce
853	Black	Wendy
860	Owens	Tim
880	Robinson	Tammy
898	Gerdes	Kate
964	Gould	Ramon
1045	Owens	Jean
1125	Bollman	Owen
1149	Becker	Owen
1123	Corner	Kathy
\.
--
-- Data for TOC Entry ID 122 (OID 3574043)
--
-- Name: book_queue Type: TABLE DATA Owner: postgres
--


COPY "book_queue"  FROM stdin;
Learning Python	7805	4	t
Perl Cookbook	7806	4	t
\.
--
-- Data for TOC Entry ID 123 (OID 3574983)
--
-- Name: stock_backup Type: TABLE DATA Owner: postgres
--


COPY "stock_backup"  FROM stdin;
0385121679	29.00	36.95	65
039480001X	30.00	32.95	31
0394800753	16.00	16.95	0
0394900014	23.00	23.95	0
044100590X	36.00	45.95	89
0441172717	17.00	21.95	77
0451160916	24.00	28.95	22
0451198492	36.00	46.95	0
0451457994	17.00	22.95	0
0590445065	23.00	23.95	10
0679803335	20.00	24.95	18
0694003611	25.00	28.95	50
0760720002	18.00	23.95	28
0823015505	26.00	28.95	16
0929605942	19.00	21.95	25
1885418035	23.00	24.95	77
\.
--
-- Data for TOC Entry ID 124 (OID 3628247)
--
-- Name: favorite_books Type: TABLE DATA Owner: manager
--


COPY "favorite_books"  FROM stdin;
102	{"The Hitchhiker's Guide to the Galaxy","The Restauraunt at the End of the Universe"}
103	{"There and Back Again: A Hobbit's Holiday","Kittens Squared"}
\.
--
-- Data for TOC Entry ID 125 (OID 3628899)
--
-- Name: employees Type: TABLE DATA Owner: postgres
--


COPY "employees"  FROM stdin;
101	Appel	Vincent
102	Holloway	Michael
105	Connoly	Sarah
104	Noble	Ben
103	Joble	David
106	Hall	Timothy
1008	Williams	\N
\.
--
-- Data for TOC Entry ID 126 (OID 3629174)
--
-- Name: editions Type: TABLE DATA Owner: manager
--


COPY "editions"  FROM stdin;
039480001X	1608	1	59	1957-03-01	h
0451160916	7808	1	75	1981-08-01	p
0394800753	1590	1	59	1949-03-01	p
0590445065	25908	1	150	1987-03-01	p
0694003611	1501	1	65	1947-03-04	p
0679803335	1234	1	102	1922-01-01	p
0760720002	190	1	91	1868-01-01	p
0394900014	1608	1	59	1957-01-01	p
0385121679	7808	2	75	1993-10-01	h
1885418035	156	1	163	1995-03-28	p
0929605942	156	2	171	1998-12-01	p
0441172717	4513	2	99	1998-09-01	p
044100590X	4513	3	99	1999-10-01	h
0451457994	4267	3	101	2000-09-12	p
0451198492	4267	3	101	1999-10-01	h
0823015505	2038	1	62	1958-01-01	p
0596000855	41473	2	113	2001-03-01	p
\.
--
-- Data for TOC Entry ID 127 (OID 3629264)
--
-- Name: books Type: TABLE DATA Owner: manager
--


COPY "books"  FROM stdin;
7808	The Shining	4156	9
4513	Dune	1866	15
4267	2001: A Space Odyssey	2001	15
1608	The Cat in the Hat	1809	2
1590	Bartholomew and the Oobleck	1809	2
25908	Franklin in the Dark	15990	2
1501	Goodnight Moon	2031	2
190	Little Women	16	6
1234	The Velveteen Rabbit	25041	3
2038	Dynamic Anatomy	1644	0
156	The Tell-Tale Heart	115	9
41473	Programming Python	7805	4
41477	Learning Python	7805	4
41478	Perl Cookbook	7806	4
41472	Practical PostgreSQL	1212	4
\.
--
-- Data for TOC Entry ID 128 (OID 3629424)
--
-- Name: distinguished_authors Type: TABLE DATA Owner: manager
--


COPY "distinguished_authors"  FROM stdin;
25043	Simon	Neil	Pulitzer Prize
1809	Geisel	Theodor Seuss	Pulitzer Prize
\.
--
-- Data for TOC Entry ID 129 (OID 3727889)
--
-- Name: favorite_authors Type: TABLE DATA Owner: manager
--


COPY "favorite_authors"  FROM stdin;
102	{{"J.R.R. Tolkien","The Silmarillion"},{"Charles Dickens","Great Expectations"},{"Ariel Denham","Attic Lives"}}
\.
--
-- Data for TOC Entry ID 130 (OID 3751599)
--
-- Name: text_sorting Type: TABLE DATA Owner: postgres
--


COPY "text_sorting"  FROM stdin;
0
1
2
3
A
B
C
D
a
b
c
d
\.
--
-- Data for TOC Entry ID 131 (OID 3751882)
--
-- Name: subjects Type: TABLE DATA Owner: postgres
--


COPY "subjects"  FROM stdin;
0	Arts	Creativity St
1	Business	Productivity Ave
2	Children's Books	Kids Ct
3	Classics	Academic Rd
4	Computers	Productivity Ave
5	Cooking	Creativity St
6	Drama	Main St
7	Entertainment	Main St
8	History	Academic Rd
9	Horror	Black Raven Dr
10	Mystery	Black Raven Dr
11	Poetry	Sunset Dr
12	Religion	\N
13	Romance	Main St
14	Science	Productivity Ave
15	Science Fiction	Main St
\.
--
-- Data for TOC Entry ID 132 (OID 3751975)
--
-- Name: alternate_stock Type: TABLE DATA Owner: postgres
--


COPY "alternate_stock"  FROM stdin;
0385121679	29.00	36.95	65
039480001X	30.00	32.95	31
0394900014	23.00	23.95	0
044100590X	36.00	45.95	89
0441172717	17.00	21.95	77
0451160916	24.00	28.95	22
0451198492	36.00	46.95	0
0451457994	17.00	22.95	0
0590445065	23.00	23.95	10
0679803335	20.00	24.95	18
0694003611	25.00	28.95	50
0760720002	18.00	23.95	28
0823015505	26.00	28.95	16
0929605942	19.00	21.95	25
1885418035	23.00	24.95	77
0394800753	16.00	16.95	4
\.
--
-- Data for TOC Entry ID 133 (OID 3752020)
--
-- Name: book_backup Type: TABLE DATA Owner: postgres
--


COPY "book_backup"  FROM stdin;
7808	The Shining	4156	9
4513	Dune	1866	15
4267	2001: A Space Odyssey	2001	15
1608	The Cat in the Hat	1809	2
1590	Bartholomew and the Oobleck	1809	2
25908	Franklin in the Dark	15990	2
1501	Goodnight Moon	2031	2
190	Little Women	16	6
1234	The Velveteen Rabbit	25041	3
2038	Dynamic Anatomy	1644	0
156	The Tell-Tale Heart	115	9
41472	Practical PostgreSQL	1212	4
41473	Programming Python	7805	4
41477	Learning Python	7805	4
41478	Perl Cookbook	7806	4
7808	The Shining	4156	9
4513	Dune	1866	15
4267	2001: A Space Odyssey	2001	15
1608	The Cat in the Hat	1809	2
1590	Bartholomew and the Oobleck	1809	2
25908	Franklin in the Dark	15990	2
1501	Goodnight Moon	2031	2
190	Little Women	16	6
1234	The Velveteen Rabbit	25041	3
2038	Dynamic Anatomy	1644	0
156	The Tell-Tale Heart	115	9
41473	Programming Python	7805	4
41477	Learning Python	7805	4
41478	Perl Cookbook	7806	4
41472	Practical PostgreSQL	1212	4
\.
--
-- Data for TOC Entry ID 134 (OID 4063343)
--
-- Name: schedules Type: TABLE DATA Owner: postgres
--


COPY "schedules"  FROM stdin;
102	Mon - Fri, 9am - 5pm
\.
COMMIT;

