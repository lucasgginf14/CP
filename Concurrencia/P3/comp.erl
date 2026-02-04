-module(comp).

-export([comp/1, comp/2, decomp/1, decomp/2, comp_proc/2, comp_proc/3, comp_proc2/5, comp_loop_ej1/3, decomp_loop_ej2/3, decomp_proc/2, decomp_proc/3]). %%%%%


-define(DEFAULT_CHUNK_SIZE, 1024*1024).

%%% File Compression
comp(File) -> %% Compress file to file.ch
    comp(File, ?DEFAULT_CHUNK_SIZE).

comp(File, Chunk_Size) ->  %% Starts a reader and a writer which run in separate processes
    case file_service:start_file_reader(File, Chunk_Size) of
        {ok, Reader} ->
            case archive:start_archive_writer(File++".ch") of
                {ok, Writer} ->
                    comp_loop(Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

comp_loop(Reader, Writer) ->  %% Compression loop => get a chunk, compress it, send to writer
    Reader ! {get_chunk, self()},  %% request a chunk from the file reader
    receive
        {chunk, Num, Offset, Data} ->   %% got one, compress and send to writer
            Comp_Data = compress:compress(Data),
            Writer ! {add_chunk, Num, Offset, Comp_Data},
            comp_loop(Reader, Writer);
        eof ->  %% end of file, stop reader and writer
            Writer ! stop;
        {error, Reason} ->
            io:format("Error reading input file: ~w~n",[Reason]),
            Reader ! stop,
            Writer ! abort
    end.
%EJERCICIO 1

comp_proc(File, Procs)->
    comp_proc(File, ?DEFAULT_CHUNK_SIZE, Procs).

comp_proc(File , Chunk_Size , Procs) ->
    case file_service:start_file_reader(File, Chunk_Size) of
        {ok, Reader}->
            case archive:start_archive_writer(File++".ch") of
                {ok , Writer} ->
                    comp_proc2(Reader, Writer, Procs, Procs, []);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.


comp_proc2(Reader, Writer, ListLength, Procs, Lista) ->
    if ListLength > 0 ->
        %Creamos un proceso y lo añadimos a la lista de procesos
        Pid = spawn(?MODULE, comp_loop_ej1, [Reader, Writer, self()]),
        %Creamos recursivamente el resto de procesos
        comp_proc2(Reader, Writer, ListLength - 1, Procs, [Pid | Lista]);
        true ->
            %Una vez creados, empezamos a controlarlos
            aux_ej1(Reader, Writer, Procs, Lista)
    end.

%Caso base: No quedan procesos y la lista está vacía
aux_ej1(Reader, Writer, 0, []) ->
    Reader ! stop,
    Writer ! stop;

aux_ej1(Reader, Writer, Procs, Lista) ->
    receive
        {ok, Pid} ->
            %Eliminamos de la lista el proceso que terminó
            aux_ej1(Reader, Writer, Procs - 1, Lista -- [Pid]);
        {error, _} ->
            %En caso de error, detenemos todos los procesos restantes y abortamos la operación
            lists:foreach(fun(P) -> P ! stop end, Lista),
            Writer ! abort,
            Reader ! stop,
            io:format("Abortando por fallo en un proceso.~n", [])
    end.




comp_loop_ej1(Reader, Writer, PidOrg) ->  %% Compression loop => get a chunk, compress it, send to writer        %%lanzar n processos q ejecuten comp_loop (bucle, esperar a q todos devuelvan eof para parar)
    Reader ! {get_chunk, self()},  %% request a chunk from the file reader
    receive
        {chunk, Num, Offset, Data} ->   %% got one, compress and send to writer
            Comp_Data = compress:compress(Data),
            Writer ! {add_chunk, Num, Offset, Comp_Data},
            comp_loop_ej1(Reader, Writer, PidOrg);
        eof ->  %% end of file, stop reader and writer
            PidOrg ! {ok, self()}; %NO ERA STOP, ERA OK
        {error, Reason} ->
            io:format("Error reading input file: ~w~n",[Reason]),
            PidOrg ! {error, self()} %NO ERA ABORT, ERA ERROR
    end.

%% File Decompression

decomp(Archive) ->
    decomp(Archive, string:replace(Archive, ".ch", "", trailing)).

decomp(Archive, Output_File) ->
    case archive:start_archive_reader(Archive) of
        {ok, Reader} ->
            case file_service:start_file_writer(Output_File) of
                {ok, Writer} ->
                    decomp_loop(Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.



decomp_loop(Reader, Writer) ->                                                  %%same shit del otro pero en este
    Reader ! {get_chunk, self()},  %% request a chunk from the reader
    receive
        {chunk, _Num, Offset, Comp_Data} ->  %% got one
            Data = compress:decompress(Comp_Data),
            Writer ! {write_chunk, Offset, Data},
            decomp_loop(Reader, Writer);
        eof ->    %% end of file => exit decompression
            Reader ! stop,
            Writer ! stop;
        {error, Reason} ->
            io:format("Error reading input file: ~w~n", [Reason]),
            Writer ! abort,
            Reader ! stop
    end.



%EJERCICIO 2

decomp_proc(Archive, Procs) ->
    decomp_proc(Archive, string:replace(Archive, ".ch", "", trailing), Procs).

decomp_proc(Archive, Output_File, Procs) ->
    case archive: start_archive_reader(Archive) of
        {ok, Reader} ->
            case file_service:start_file_writer(Output_File) of
                {ok, Writer} ->
                    decomp_proc2(Reader, Writer, Procs, Procs, []);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

decomp_proc2(Reader, Writer, ListLength, Procs, Lista) ->
    if ListLength > 0 ->
        %Creamos un proceso y lo añadimos a la lista de procesos
        Pid = spawn(?MODULE, decomp_loop_ej2, [Reader, Writer, self()]),
        %Creamos recursivamente el resto de procesos
        decomp_proc2(Reader, Writer, ListLength - 1, Procs, [Pid | Lista]);
        true ->
            %Una vez creados, empezamos a controlarlos
            aux_ej2(Reader, Writer, Procs, Lista)
    end.

%Caso base: No quedan procesos y la lista está vacía
aux_ej2(Reader, Writer, 0, []) ->
    Reader ! stop,
    Writer ! stop;

aux_ej2(Reader, Writer, Procs, Lista) ->
    receive
        {ok, Pid} ->
            %Eliminamos de la lista el proceso que terminó
            aux_ej2(Reader, Writer, Procs - 1, Lista -- [Pid]);
        {error, _} ->
            %En caso de error, detenemos todos los procesos restantes y abortamos la operación
            lists:foreach(fun(P) -> P ! stop end, Lista),
            Writer ! abort,
            Reader ! stop,
            io:format("Abortando por fallo en un proceso.~n", [])
    end.

decomp_loop_ej2(Reader, Writer, PidOrg) ->
    Reader ! {get_chunk, self()},
    receive
        {chunk, _Num, Offset, Comp_Data} ->
            Data = compress:decompress(Comp_Data),
            Writer ! {write_chunk, Offset, Data},
            decomp_loop_ej2(Reader, Writer, PidOrg);
        eof ->
            PidOrg ! {ok, self()};
        {error, Reason} ->
            io:format("Error reading input file: ~w~n", [Reason]),
            PidOrg ! {error, self()}
    end.
