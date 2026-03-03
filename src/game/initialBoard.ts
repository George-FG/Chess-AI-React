import type { Board, Piece } from './types';

export const createInitialBoard = (): Board => {
  const board: Board = Array(8).fill(null).map(() => Array(8).fill(null));

  // White pieces (row 0 and 1)
  const backRow: Piece[] = [
    { type: 'r', color: 'white' }, // a1
    { type: 'n', color: 'white' }, // b1
    { type: 'b', color: 'white' }, // c1
    { type: 'k', color: 'white' }, // d1 - King on d-file (displays as e1 due to column reversal)
    { type: 'q', color: 'white' }, // e1 - Queen on e-file (displays as d1 due to column reversal)
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
    { type: 'k', color: 'black' }, // d8 - King on d-file (displays as e8 due to column reversal)
    { type: 'q', color: 'black' }, // e8 - Queen on e-file (displays as d8 due to column reversal)
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
