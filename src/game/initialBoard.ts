import type { Board, Piece } from './types';

export const createInitialBoard = (): Board => {
  const board: Board = Array(8).fill(null).map(() => Array(8).fill(null));

  // White pieces (row 0 and 1)
  const backRow: Piece[] = [
    { type: 'r', color: 'white' }, // a1
    { type: 'n', color: 'white' }, // b1
    { type: 'b', color: 'white' }, // c1
    { type: 'q', color: 'white' }, // d1 - Queen on d-file
    { type: 'k', color: 'white' }, // e1 - King on e-file
    { type: 'b', color: 'white' }, // f1
    { type: 'n', color: 'white' }, // g1
    { type: 'r', color: 'white' }, // h1
  ];

  backRow.forEach((piece, col) => {
    board[0][col] = piece;
  });

  for (let col = 0; col < 8; col++) {
    board[1][col] = { type: 'p', color: 'white' };
  }

  // Black pieces (row 6 and 7)
  const blackBackRow: Piece[] = [
    { type: 'r', color: 'black' }, // a8
    { type: 'n', color: 'black' }, // b8
    { type: 'b', color: 'black' }, // c8
    { type: 'q', color: 'black' }, // d8 - Queen on d-file
    { type: 'k', color: 'black' }, // e8 - King on e-file
    { type: 'b', color: 'black' }, // f8
    { type: 'n', color: 'black' }, // g8
    { type: 'r', color: 'black' }, // h8
  ];

  blackBackRow.forEach((piece, col) => {
    board[7][col] = piece;
  });

  for (let col = 0; col < 8; col++) {
    board[6][col] = { type: 'p', color: 'black' };
  }

  return board;
};
