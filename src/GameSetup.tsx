import React, { useState } from "react";
import "./GameSetup.css";

export type PlayerType = "human" | "ai";

export interface AISettings {
  depth: number;
  maxTime: number; // in milliseconds
}

export interface GameSetupOptions {
  whitePlayer: PlayerType;
  blackPlayer: PlayerType;
  whiteAI?: AISettings;
  blackAI?: AISettings;
  clockEnabled: boolean;
  initialTime: number;
}

interface GameSetupProps {
  onStartGame: (options: GameSetupOptions) => void;
}

const DEPTH_OPTIONS = [
  { label: "1 - Very Easy", value: 1 },
  { label: "2 - Easy", value: 2 },
  { label: "3 - Medium", value: 3 },
  { label: "4 - Hard", value: 4 },
  { label: "5 - Very Hard", value: 5 },
  { label: "6 - Expert", value: 6 },
];

const CLOCK_TIME_OPTIONS = [
  { label: "30 seconds", value: 30 },
  { label: "1 minute", value: 60 },
  { label: "2 minutes", value: 120 },
  { label: "3 minutes", value: 180 },
  { label: "5 minutes", value: 300 },
  { label: "10 minutes", value: 600 },
  { label: "15 minutes", value: 900 },
  { label: "30 minutes", value: 1800 },
];

const MAX_TIME_OPTIONS = [
  { label: "1 second", value: 1000 },
  { label: "2 seconds", value: 2000 },
  { label: "3 seconds", value: 3000 },
  { label: "5 seconds", value: 5000 },
  { label: "10 seconds", value: 10000 },
  { label: "15 seconds", value: 15000 },
  { label: "30 seconds", value: 30000 },
  { label: "No limit", value: 0 },
];

const GameSetup: React.FC<GameSetupProps> = ({ onStartGame }) => {
  const [whitePlayer, setWhitePlayer] = useState<PlayerType>("human");
  const [blackPlayer, setBlackPlayer] = useState<PlayerType>("ai");
  const [whiteAIDepth, setWhiteAIDepth] = useState<number>(3);
  const [blackAIDepth, setBlackAIDepth] = useState<number>(3);
  const [whiteAIMaxTime, setWhiteAIMaxTime] = useState<number>(5000);
  const [blackAIMaxTime, setBlackAIMaxTime] = useState<number>(5000);
  const [clockEnabled, setClockEnabled] = useState<boolean>(false);
  const [initialTime, setInitialTime] = useState<number>(900);

  const handleStartGame = () => {
    onStartGame({
      whitePlayer,
      blackPlayer,
      whiteAI: whitePlayer === "ai" ? { 
        depth: whiteAIDepth,
        maxTime: whiteAIMaxTime
      } : undefined,
      blackAI: blackPlayer === "ai" ? { 
        depth: blackAIDepth,
        maxTime: blackAIMaxTime
      } : undefined,
      clockEnabled,
      initialTime,
    });
  };

  return (
    <div className="game-setup-container">
      <div className="game-setup-card">
        <h1>Chess Game Setup</h1>
        
        <div className="player-selection">
          <div className="player-option">
            <h2>White Player</h2>
            <div className="radio-group">
              <label className={whitePlayer === "human" ? "selected" : ""}>
                <input
                  type="radio"
                  name="white"
                  value="human"
                  checked={whitePlayer === "human"}
                  onChange={() => setWhitePlayer("human")}
                />
                <span>Human</span>
              </label>
              <label className={whitePlayer === "ai" ? "selected" : ""}>
                <input
                  type="radio"
                  name="white"
                  value="ai"
                  checked={whitePlayer === "ai"}
                  onChange={() => setWhitePlayer("ai")}
                />
                <span>AI</span>
              </label>
            </div>
          </div>

          <div className="vs-divider">VS</div>

          <div className="player-option">
            <h2>Black Player</h2>
            <div className="radio-group">
              <label className={blackPlayer === "human" ? "selected" : ""}>
                <input
                  type="radio"
                  name="black"
                  value="human"
                  checked={blackPlayer === "human"}
                  onChange={() => setBlackPlayer("human")}
                />
                <span>Human</span>
              </label>
              <label className={blackPlayer === "ai" ? "selected" : ""}>
                <input
                  type="radio"
                  name="black"
                  value="ai"
                  checked={blackPlayer === "ai"}
                  onChange={() => setBlackPlayer("ai")}
                />
                <span>AI</span>
              </label>
            </div>
          </div>
        </div>

        {(whitePlayer === "ai" || blackPlayer === "ai") && (
          <div className="ai-section">
            <div className="ai-settings-grid">
              {whitePlayer === "ai" && (
                <div className="ai-settings">
                  <div className="ai-header">
                    <h2>⚪ White AI</h2>
                  </div>
                  
                  <div className="ai-setting-group">
                    <label>Difficulty (Search Depth)</label>
                    <select
                      value={whiteAIDepth}
                      onChange={(e) => setWhiteAIDepth(Number(e.target.value))}
                      className="depth-select"
                    >
                      {DEPTH_OPTIONS.map((option) => (
                        <option key={option.value} value={option.value}>
                          {option.label}
                        </option>
                      ))}
                    </select>
                    <small className="help-text">
                      Higher depth = stronger AI but slower moves
                    </small>
                  </div>

                  <div className="ai-setting-group">
                    <label>Max Move Time</label>
                    <select
                      value={whiteAIMaxTime}
                      onChange={(e) => setWhiteAIMaxTime(Number(e.target.value))}
                      className="time-select"
                    >
                      {MAX_TIME_OPTIONS.map((option) => (
                        <option key={option.value} value={option.value}>
                          {option.label}
                        </option>
                      ))}
                    </select>
                    <small className="help-text">
                      Maximum time AI can spend per move
                    </small>
                  </div>
                </div>
              )}

              {blackPlayer === "ai" && (
                <div className="ai-settings">
                  <div className="ai-header">
                    <h2>⚫ Black AI</h2>
                  </div>
                  
                  <div className="ai-setting-group">
                    <label>Difficulty (Search Depth)</label>
                    <select
                      value={blackAIDepth}
                      onChange={(e) => setBlackAIDepth(Number(e.target.value))}
                      className="depth-select"
                    >
                      {DEPTH_OPTIONS.map((option) => (
                        <option key={option.value} value={option.value}>
                          {option.label}
                        </option>
                      ))}
                    </select>
                    <small className="help-text">
                      Higher depth = stronger AI but slower moves
                    </small>
                  </div>

                  <div className="ai-setting-group">
                    <label>Max Move Time</label>
                    <select
                      value={blackAIMaxTime}
                      onChange={(e) => setBlackAIMaxTime(Number(e.target.value))}
                      className="time-select"
                    >
                      {MAX_TIME_OPTIONS.map((option) => (
                        <option key={option.value} value={option.value}>
                          {option.label}
                        </option>
                      ))}
                    </select>
                    <small className="help-text">
                      Maximum time AI can spend per move
                    </small>
                  </div>
                </div>
              )}
            </div>
          </div>
        )}

        <div className="clock-section">
          <h2>⏱️ Chess Clock</h2>
          <div className="clock-toggle">
            <label>
              <input
                type="checkbox"
                checked={clockEnabled}
                onChange={(e) => setClockEnabled(e.target.checked)}
              />
              <span>Enable Chess Clock</span>
            </label>
          </div>
          {clockEnabled && (
            <div className="clock-time-selector">
              <label>Time per Player</label>
              <select
                value={initialTime}
                onChange={(e) => setInitialTime(Number(e.target.value))}
              >
                {CLOCK_TIME_OPTIONS.map((option) => (
                  <option key={option.value} value={option.value}>
                    {option.label}
                  </option>
                ))}
              </select>
            </div>
          )}
        </div>

        <button className="start-button" onClick={handleStartGame}>
          Start Game
        </button>
      </div>
    </div>
  );
};

export default GameSetup;
